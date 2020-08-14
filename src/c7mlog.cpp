/*
 * c7mlog.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include "c7file.hpp"
#include "c7mlog.hpp"
#include "c7path.hpp"
#include "c7thread.hpp"
#include <sys/mman.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <stdexcept>


// BEGIN: same definition with c7mlog.[ch]

// _REVISION history:
// 1: initial
// 2: lnk data lnk -> lnk data thread_name lnk
// 3: thread name, source name
// 4: record max length for thread name and source name
#define _REVISION	4

#define _IHDRSIZE		c7_align(sizeof(hdr_t), 16)

// lnk_t internal control flags (6bits)
#define _LNK_CONTROL_CHOICE	(1U << 0)

#define _TN_MAX			(63)	// tn_size:6
#define _SN_MAX			(63)	// sn_size:6

// header part
struct hdr_t {
    uint32_t rev;
    uint32_t cnt;
    uint32_t hdrsize_b;		// user header size
    uint32_t logsize_b;		// ring buffer size
    uint32_t nextlnkaddr;	// ring addrress to next lnk_t
    char hint[64];
    uint8_t max_tn_size;	// maximum length of thread name
    uint8_t max_sn_size;	// maximum length of source name
};

// linkage part
struct lnk_t {
    // prevlnkoff must be head of lnk_t (cf. comment of calling rbuf_put in update_lnk())
    uint32_t prevlnkoff;	// offset to previous record
    uint32_t nextlnkoff;	// offset to next record
    c7::usec_t time_us;		// time stamp in micro sec.
    uint64_t minidata;		// mini data (out of libc7)
    uint64_t level   :  3;	// log level 0..7
    uint64_t category:  5;	// log category 0..31
    uint64_t tn_size :  6;	// length of thread name (exclude null character)
    uint64_t sn_size :  6;	// length of source name (exclude null character)
    uint64_t src_line: 14;	// source line
    uint64_t control :  6;	// (internal control flags)
    uint64_t __rsv1  : 24;
    uint32_t th_id;		// thread id
    uint32_t order;		// record serial number
    uint32_t size;		// record size (data + xxx name)
    uint32_t __rsv2;
};

// END


namespace c7 {


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

typedef uint32_t raddr_t;

class rbuffer {
private:
    char *top_ = nullptr;
    char *end_ = nullptr;
    raddr_t size_ = 0;

public:
    rbuffer() {} 

    explicit rbuffer(void *addr, raddr_t size):
	top_(static_cast<char*>(addr)), end_(top_ + size), size_(size) {
    }

    void get(raddr_t addr, raddr_t size, void *__ubuf) {
	char *ubuf = static_cast<char*>(__ubuf);
	char *rbuf = top_ + (addr % size_);
	raddr_t rrest = end_ - rbuf;

	while (size > 0) {
	    raddr_t cpsize = std::min(size, rrest);

	    (void)std::memcpy(ubuf, rbuf, cpsize);
	    ubuf += cpsize;
	    size -= cpsize;

	    rbuf = top_;
	    rrest = size_;
	}
    }
    
    raddr_t put(raddr_t addr, raddr_t size, const void *__ubuf) {
	const raddr_t ret_addr = addr + size;

	const char *ubuf = static_cast<const char*>(__ubuf);
	char *rbuf = top_ + (addr % size_);
	raddr_t rrest = end_ - rbuf;

	while (size > 0) {
	    raddr_t cpsize = (size < rrest) ? size : rrest;

	    (void)memcpy(rbuf, ubuf, cpsize);
	    ubuf += cpsize;
	    size -= cpsize;

	    rbuf = top_;
	    rrest = size_;
	}

	return ret_addr;
    }
};


/*----------------------------------------------------------------------------
                              mlog_writer::impl
----------------------------------------------------------------------------*/

class mlog_writer::impl {
private:
    rbuffer rbuf_;
    hdr_t *hdr_ = nullptr;
    size_t mmapsize_b_;
    size_t maxdsize_b_;
    uint32_t flags_;
    c7::thread::spinlock lock_;
    char snbuf_[_SN_MAX + 1];

private:
    result<void> setup_storage(const char *path,
			       size_t hdrsize_b, size_t logsize_b);
    void setup_context(size_t hdrsize_b, size_t logsize_b, const char *hint_op);
    void free_storage();

    raddr_t update_lnk(size_t logsize, size_t tn_size, size_t sn_size, int src_line,
		       c7::usec_t time_us, uint32_t level, uint32_t category,
		       uint64_t minidata);

public:
    impl() {
	if (!init(nullptr, 0, C7_MLOG_SIZE_MIN, 0, nullptr))
	    throw std::runtime_error("cannot allocate minimal storage");
	mmapsize_b_ = 0;	// allocated by malloc instead of mmap.
    }

    ~impl() {
	free_storage();
    }

    result<void> init(const char *path,
		      size_t hdrsize_b, size_t logsize_b,
		      uint32_t w_flags, const char *hint) {
	auto res = setup_storage(path, hdrsize_b, logsize_b);
	if (!res)
	    return res;
	setup_context(hdrsize_b, logsize_b, hint);
	flags_ = w_flags;
	return c7result_ok();
    }

    bool put(c7::usec_t time_us, const char *src_name, int src_line,
	     uint32_t level, uint32_t category, uint64_t minidata,
	     const void *logaddr, size_t logsize_b);

    void clear();

    void *hdraddr(size_t *hdrsize_b_op) {
	if (hdrsize_b_op != NULL)
	    *hdrsize_b_op = hdr_->hdrsize_b;
	return (char *)hdr_ + _IHDRSIZE;
    }
};


// constructor/destructor -----------------------------------------

void
mlog_writer::impl::free_storage()
{
    if (hdr_ != nullptr) {
	if (mmapsize_b_ == 0)
	    std::free(hdr_);		// maybe allocated at default constructor
	else
	    ::munmap(hdr_, mmapsize_b_);
    }
    hdr_ = nullptr;
}

result<void>
mlog_writer::impl::setup_storage(const char *path, size_t hdrsize_b, size_t logsize_b)
{
    if (logsize_b < C7_MLOG_SIZE_MIN || logsize_b > C7_MLOG_SIZE_MAX) {
	return c7result_err(EINVAL, "invalid logsize_b: %{}", logsize_b);
    }
    mmapsize_b_ = _IHDRSIZE + hdrsize_b + logsize_b;
    maxdsize_b_ = logsize_b - 2*sizeof(lnk_t);

    void *top;
    if (path == nullptr) {
	top = std::calloc(mmapsize_b_, 1);
	if (top == nullptr)
	    return c7result_err(errno, "cannot allocate storage");
    } else {
	auto res = c7::file::mmap_rw(path, mmapsize_b_, true);
	if (!res)
	    return res;
	top = res.value().release();
    }

    free_storage();
    hdr_ = static_cast<hdr_t*>(top);
    rbuf_ = rbuffer(static_cast<char*>(top) + _IHDRSIZE + hdrsize_b, logsize_b);

    return c7result_ok();
}

void
mlog_writer::impl::setup_context(size_t hdrsize_b, size_t logsize_b, const char *hint_op)
{
    if (hdr_->hdrsize_b != hdrsize_b ||
	hdr_->logsize_b != logsize_b ||
	hdr_->rev       != _REVISION) {
	(void)std::memset(hdr_, 0, mmapsize_b_);
	hdr_->rev       = _REVISION;
	hdr_->hdrsize_b = hdrsize_b;
	hdr_->logsize_b = logsize_b;
	if (hint_op != nullptr)
	    (void)std::strncat(hdr_->hint, hint_op, sizeof(hdr_->hint) - 1);
    }

    if (hdr_->cnt == 0 && hdr_->nextlnkaddr == 0) {
	lnk_t lnk = {0};
	lnk.prevlnkoff = hdr_->logsize_b * 2;
	(void)rbuf_.put(0, sizeof(lnk), &lnk);
    }
}


// logging --------------------------------------------------------

raddr_t
mlog_writer::impl::update_lnk(size_t logsize, size_t tn_size, size_t sn_size, int src_line,
			      c7::usec_t time_us, uint32_t level, uint32_t category,
			      uint64_t minidata)
{
    lnk_t lnk;
    raddr_t const prevlnkaddr = hdr_->nextlnkaddr;
    raddr_t addr;
    
    // null character is not counted for *_size, but it's put to rbuf.
    if (tn_size > 0) {
	if (hdr_->max_tn_size < tn_size)
	    hdr_->max_tn_size = tn_size;
	logsize += (tn_size + 1);
    }
    if (sn_size > 0) {
	if (hdr_->max_sn_size < sn_size)
	    hdr_->max_sn_size = sn_size;
	logsize += (sn_size + 1);
    }

    // put lnk as a new terminator
    addr = prevlnkaddr + sizeof(lnk) + logsize;	// address of new terminator
    lnk.prevlnkoff = addr - prevlnkaddr;
    lnk.nextlnkoff = 0;				// it mean 'terminator'
    lnk.order = 0;
    lnk.control = 0;
    (void)rbuf_.put(addr, sizeof(lnk), &lnk);
    hdr_->nextlnkaddr = addr;
    
    // update previous terminator as lnk of last data
    lnk.nextlnkoff = hdr_->nextlnkaddr - prevlnkaddr;
    lnk.time_us = time_us;
    lnk.order = ++hdr_->cnt;
    lnk.size = logsize;
    lnk.th_id = c7::thread::self::id();
    lnk.tn_size = tn_size;
    lnk.sn_size = sn_size;
    lnk.src_line = src_line;
    lnk.minidata = minidata;
    lnk.level = level;
    lnk.category = category;
    lnk.control = 0;
    // next rbuf_::put re-write tail part of a lnk to KEEP prevlnkaddr.
    // this is depend on prevlnkaddr located at head of lnk.
    (void)rbuf_.put(prevlnkaddr + sizeof(lnk.prevlnkoff),
		    sizeof(lnk) - sizeof(lnk.prevlnkoff),
		    (char *)&lnk + sizeof(lnk.prevlnkoff));

    return prevlnkaddr + sizeof(lnk);	// address of data
}

bool
mlog_writer::impl::put(c7::usec_t time_us, const char *src_name, int src_line,
		       uint32_t level, uint32_t category, uint64_t minidata,
		       const void *logaddr, size_t logsize_b)
{
    // thread name size
    const char *th_name = nullptr;
    if ((flags_ & C7_MLOG_F_THREAD_NAME) != 0)
	th_name = c7::thread::self::name();
    size_t tn_size = th_name ? strlen(th_name) : 0;
    if (tn_size > _TN_MAX) {
	th_name += (tn_size - _TN_MAX);
	tn_size = _TN_MAX;
    }

    // source name size
    size_t sn_size;
    if ((flags_ & C7_MLOG_F_SOURCE_NAME) == 0 || src_name == NULL) {
	src_name = NULL;
	sn_size = 0;
    } else {
	src_name = c7path_name(src_name);
	const char *sfx = c7path_suffix(src_name);
	if ((sn_size = sfx - src_name) > _SN_MAX) {
	    src_name += (sn_size - _SN_MAX);
	    sn_size = _SN_MAX;
	}
	// DON'T setup snbuf_ here, because thread-unsafe.
    }

    // check size to be written
    if ((logsize_b + tn_size + sn_size + 2) > maxdsize_b_)
	return false;					// data size too large

    if ((flags_ & C7_MLOG_F_SPINLOCK) != 0)
	lock_._lock();

    // write two links and data (log, [thread name], [source name])
    raddr_t addr = update_lnk(logsize_b, tn_size, sn_size, src_line,	// links
			      time_us, level, category, minidata);
    addr = rbuf_.put(addr, logsize_b, logaddr);				// log data
    if (tn_size > 0)
	addr = rbuf_.put(addr, tn_size+1, th_name);			// +1 : null character
    if (sn_size > 0) {
	(void)c7::strbcpy(snbuf_, src_name, src_name + sn_size);	// snbuf_ is used to exclude suffix.
	addr = rbuf_.put(addr, sn_size+1, snbuf_);			// +1 : null character
    }
    hdr_->nextlnkaddr %= hdr_->logsize_b;

    if ((flags_ & C7_MLOG_F_SPINLOCK) != 0)
	lock_.unlock();

    return true;
}


// clear (reset) --------------------------------------------------

void
mlog_writer::impl::clear()
{
    hdr_->cnt = 0;
    hdr_->nextlnkaddr = 0;
    lnk_t lnk = {0};
    lnk.prevlnkoff = hdr_->logsize_b * 2;
    (void)rbuf_.put(0, sizeof(lnk), &lnk);
}


/*----------------------------------------------------------------------------
                                 mlog_writer
----------------------------------------------------------------------------*/

mlog_writer::mlog_writer(): pimpl(new mlog_writer::impl()) {}

mlog_writer::~mlog_writer()
{
    delete pimpl;
}

mlog_writer::mlog_writer(mlog_writer&& o): pimpl(o.pimpl)
{
    o.pimpl = nullptr;
}

mlog_writer& mlog_writer::operator=(mlog_writer&& o)
{
    if (this != &o) {
	delete pimpl;
	pimpl = o.pimpl;
	o.pimpl = nullptr;
    }
    return *this;
}

result<void>
mlog_writer::init(const std::string& name,
		  size_t hdrsize_b, size_t logsize_b,
		  uint32_t w_flags, const char *hint)
{
    auto path = c7::path::init_c7spec(name, ".mlog", C7_MLOG_DIR_ENV);
    return pimpl->init(path.c_str(), hdrsize_b, logsize_b, w_flags, hint);
}

void *mlog_writer::hdraddr(size_t *hdrsize_b_op)
{
    return pimpl->hdraddr(hdrsize_b_op);
}

bool mlog_writer::put(c7::usec_t time_us, const char *src_name, int src_line,
		      uint32_t level, uint32_t category, uint64_t minidata,
		      const void *logaddr, size_t logsize_b)
{
    return pimpl->put(time_us, src_name, src_line,
		      level, category, minidata,
		      logaddr, logsize_b);
}

bool mlog_writer::put(const char *src_name, int src_line,
		      uint32_t level, uint32_t category, uint64_t minidata,
		      const void *logaddr, size_t logsize_b)
{
    return put(c7::time_us(), src_name, src_line, level, category, minidata,
	       logaddr, logsize_b);
}

bool mlog_writer::put(const char *src_name, int src_line,
		      uint32_t level, uint32_t category, uint64_t minidata,
		      const std::string& s)
{
    return put(c7::time_us(), src_name, src_line, level, category, minidata,
	       static_cast<const void *>(s.c_str()), s.size()+1);
}

bool mlog_writer::put(const char *src_name, int src_line,
		      uint32_t level, uint32_t category, uint64_t minidata,
		      const char *s)
{
    return put(c7::time_us(), src_name, src_line, level, category, minidata,
	       static_cast<const void *>(s), strlen(s)+1);
}
    

/*----------------------------------------------------------------------------
                              mlog_reader::impl
----------------------------------------------------------------------------*/

class mlog_reader::impl {
private:
    rbuffer rbuf_;
    hdr_t *hdr_ = nullptr;
    std::vector<char> dbuf_;
    info_t info_;

    raddr_t find_origin(size_t maxcount,
			uint32_t order_min,
			c7::usec_t time_us_min,
			std::function<bool(const info_t&)> choice);

public:
    impl() {}

    ~impl() {
	std::free(hdr_);
    }

    result<void> load(const std::string& path);

    void scan(size_t maxcount,
	      uint32_t order_min,
	      c7::usec_t time_us_min,
	      std::function<bool(const info_t&)> choice,
	      std::function<bool(const info_t&, void*)> access);

    void *hdraddr(size_t *hdrsize_b_op) {
	if (hdrsize_b_op != NULL)
	    *hdrsize_b_op = hdr_->hdrsize_b;
	return (char *)hdr_ + _IHDRSIZE;
    }

    int thread_name_size() {
	return hdr_->max_tn_size;
    }

    int source_name_size() {
	return hdr_->max_sn_size;
    }

    const char *hint() {
	return hdr_->hint;
    }
};

static void
make_info(mlog_reader::info_t& info, const lnk_t& lnk, const char *data)
{
    info.thread_id   = lnk.th_id;
    info.source_line = lnk.src_line;
    info.order       = lnk.order;
    info.size_b      = lnk.size;
    info.time_us     = lnk.time_us;
    info.level       = lnk.level;
    info.category    = lnk.category;
    info.minidata    = lnk.minidata;

    if (lnk.sn_size > 0) {
	info.size_b -= (lnk.sn_size + 1);
	info.source_name = data + info.size_b;
    } else
	info.source_name = "";

    if (lnk.tn_size > 0) {
	info.size_b -= (lnk.tn_size + 1);
	info.thread_name = data + info.size_b;
    } else
	info.thread_name = "";
}

result<void>
mlog_reader::impl::load(const std::string& path)
{
    auto res = c7::file::read(path);
    if (!res)
	return res;

    void *top = res.value().release();
    hdr_ = static_cast<hdr_t*>(top);
    rbuf_ = rbuffer(static_cast<char*>(top) + _IHDRSIZE + hdr_->hdrsize_b,
		    hdr_->logsize_b);

    return c7result_ok();
}

raddr_t
mlog_reader::impl::find_origin(size_t maxcount,
			       uint32_t order_min,
			       c7::usec_t time_us_min,
			       std::function<bool(const info_t&)> choice)
{
    raddr_t rewindsize = sizeof(lnk_t);
    raddr_t lnkaddr = hdr_->nextlnkaddr + hdr_->logsize_b * 2;
    lnk_t lnk;
    
    if (maxcount == 0)
	maxcount = -1UL;

    rbuf_.get(lnkaddr, sizeof(lnk), &lnk);
    rewindsize += lnk.prevlnkoff;
    lnkaddr -= lnk.prevlnkoff;

    for (;;) {
	rbuf_.get(lnkaddr, sizeof(lnk), &lnk);
	rewindsize += lnk.prevlnkoff;

	dbuf_.clear();				// data.capacity() is not changed.
	dbuf_.reserve(lnk.size);
	rbuf_.get(lnkaddr + sizeof(lnk), lnk.size, dbuf_.data());
	make_info(info_, lnk, dbuf_.data());

	lnk.control &= ~_LNK_CONTROL_CHOICE;	// clear choiced flag previously stored
	if (lnk.nextlnkoff != 0 && choice(info_)) {
	    lnk.control |= _LNK_CONTROL_CHOICE;
	    maxcount--;
	}
	(void)rbuf_.put(lnkaddr, sizeof(lnk), &lnk);	// IMPORTANT

	if (maxcount == 0)
	    break;
	if (lnk.order < order_min || lnk.time_us < time_us_min)
	    break;
	if (rewindsize > hdr_->logsize_b)
	    break;
	if (lnk.prevlnkoff <= 0)
	    break;				// might be broken
	lnkaddr -= lnk.prevlnkoff;
    }

    return lnkaddr;
}

void
mlog_reader::impl::scan(size_t maxcount,
			uint32_t order_min,
			c7::usec_t time_us_min,
			std::function<bool(const info_t&)> choice,
			std::function<bool(const info_t&, void*)> access)
{
    raddr_t addr = find_origin(maxcount, order_min, time_us_min, choice);

    for (;;) {
	lnk_t lnk;
	rbuf_.get(addr, sizeof(lnk), &lnk);
	if (lnk.nextlnkoff == 0)		// terminator
	    break;
	addr += sizeof(lnk);			// data address
	if ((lnk.control & _LNK_CONTROL_CHOICE) != 0) {
	    dbuf_.clear();		// data.capacity() is not changed.
	    dbuf_.reserve(lnk.size);
	    rbuf_.get(addr, lnk.size, dbuf_.data());
	    make_info(info_, lnk, dbuf_.data());
	    if (!access(info_, dbuf_.data()))
		break;
	}
	addr += lnk.size;			// next lnk address
    }
}


/*----------------------------------------------------------------------------
                                 mlog_reader
----------------------------------------------------------------------------*/

mlog_reader::mlog_reader(): pimpl(new mlog_reader::impl()) {}

mlog_reader::~mlog_reader()
{
    delete pimpl;
}

mlog_reader::mlog_reader(mlog_reader&& o): pimpl(o.pimpl)
{
    o.pimpl = nullptr;
}

mlog_reader& mlog_reader::operator=(mlog_reader&& o)
{
    if (this != &o) {
	delete pimpl;
	pimpl = o.pimpl;
	o.pimpl = nullptr;
    }
    return *this;
}

result<void>
mlog_reader::load(const std::string& name)
{
    auto path = c7::path::find_c7spec(name, ".mlog", C7_MLOG_DIR_ENV);
    return pimpl->load(path);
}

void
mlog_reader::scan(size_t maxcount,
		  uint32_t order_min,
		  c7::usec_t time_us_min,
		  std::function<bool(const info_t& info)> choice,
		  std::function<bool(const info_t& info, void *data)> access)
{
    pimpl->scan(maxcount, order_min, time_us_min, choice, access);
}

int mlog_reader::thread_name_size()
{
    return pimpl->thread_name_size();
}

int mlog_reader::source_name_size()
{
    return pimpl->source_name_size();
}

const char *mlog_reader::hint()
{
    return pimpl->hint();
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7
