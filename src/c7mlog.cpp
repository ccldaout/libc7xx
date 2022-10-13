/*
 * c7mlog.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <c7file.hpp>
#include <c7mlog.hpp>
#include <c7path.hpp>
#include <c7thread.hpp>
#include <sys/mman.h>
#include <unistd.h>


// BEGIN: same definition with c7mlog.[ch]

// _REVISION history:
// 1: initial
// 2: lnk data lnk -> lnk data thread_name lnk
// 3: thread name, source name
// 4: record max length for thread name and source name
// 5: new linkage format & enable inter process lock
// 6: lock free, no max_{tn|sn}_size
#define _REVISION		(6)

#define _IHDRSIZE		c7_align(sizeof(hdr_t), 16)
#define _DUMMY_LOG_SIZE		(16)

// rec_t internal control flags (6bits)
#define _REC_CONTROL_CHOICE	(1U << 0)

#define _TN_MAX			(63)	// tn_size:6
#define _SN_MAX			(63)	// sn_size:6

typedef uint32_t raddr_t;

// file header
struct hdr_t {
    uint32_t rev;
    volatile raddr_t nextaddr;
    volatile uint32_t cnt;
    raddr_t logsize_b;		// ring buffer size
    uint32_t hdrsize_b;		// user header size
    char hint[64];
};

// record header
struct rec_t {
    raddr_t size;		// record size (rec_t + log data + names + raddr)
    uint32_t order;		// (weak) record order
    c7::usec_t time_us;		// time stamp in micro sec.
    uint64_t minidata;		// mini data (out of libc7)
    uint64_t level   :  3;	// log level 0..7
    uint64_t category:  5;	// log category 0..31
    uint64_t tn_size :  6;	// length of thread name (exclude null character)
    uint64_t sn_size :  6;	// length of source name (exclude null character)
    uint64_t src_line: 14;	// source line
    uint64_t control :  6;	// (internal control flags)
    uint64_t __rsv1  : 24;
    uint32_t pid;		// process id
    uint32_t th_id;		// thread id
    uint32_t br_order;		// ~order
};

// END


namespace c7 {


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

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

static char _DummyBuffer[_IHDRSIZE + _DUMMY_LOG_SIZE];

class mlog_writer::impl {
private:
    callback_t callback_;
    rbuffer rbuf_;
    hdr_t *hdr_ = nullptr;
    size_t mmapsize_b_;			// 0:_DummyBuffer, >0:mmaped storage
    uint32_t flags_;
    uint32_t pid_;

private:
    result<> setup_storage(const char *path,
			   size_t hdrsize_b, size_t logsize_b);
    void setup_context(size_t hdrsize_b, size_t logsize_b, const char *hint_op);
    void free_storage();

    rec_t make_rechdr(size_t logsize, size_t tn_size, size_t sn_size, int src_line,
		      c7::usec_t time_us, uint32_t level, uint32_t category,
		      uint64_t minidata);

public:
    impl() {
	init(nullptr, 0, _DUMMY_LOG_SIZE, 0, nullptr);
    }

    ~impl() {
	free_storage();
    }

    result<> init(const char *path, size_t hdrsize_b, size_t logsize_b,
		  uint32_t w_flags, const char *hint);

    void set_callback(callback_t callback) {
	callback_ = callback;
    }

    bool put(c7::usec_t time_us, const char *src_name, int src_line,
	     uint32_t level, uint32_t category, uint64_t minidata,
	     const void *logaddr, size_t logsize_b);

    void clear();

    void *hdraddr(size_t *hdrsize_b_op) {
	if (hdrsize_b_op != nullptr) {
	    *hdrsize_b_op = hdr_->hdrsize_b;
	}
	return (char *)hdr_ + _IHDRSIZE;
    }

    void post_forked() {
	pid_ = getpid();
    }
};


static void print_stdout(c7::usec_t time_us, const char *src_name, int src_line,
			 uint32_t level, uint32_t category, uint64_t minidata,
			 const void *logaddr, size_t logsize_b)
{
    src_name = c7path_name(src_name);
    if (auto n = std::strlen(src_name); n > 16) {
	src_name += (n - 16);
    }
    c7::p_("%{t} %{>16}:%{03} @%{02}: %{}",
	   time_us, src_name, src_line, c7::thread::self::id(), static_cast<const char *>(logaddr));
}


// constructor/destructor -----------------------------------------

result<>
mlog_writer::impl::init(const char *path,
			size_t hdrsize_b, size_t logsize_b,
			uint32_t w_flags, const char *hint)
{
    auto res = setup_storage(path, hdrsize_b, logsize_b);
    if (!res) {
	return res;
    }
    setup_context(hdrsize_b, logsize_b, hint);
    flags_ = w_flags;
    pid_ = getpid();
    return c7result_ok();
}

result<>
mlog_writer::impl::setup_storage(const char *path, size_t hdrsize_b, size_t logsize_b)
{
    mmapsize_b_ = _IHDRSIZE + hdrsize_b + logsize_b;

    void *top;
    if (path == nullptr) {
	top = _DummyBuffer;
	mmapsize_b_ = 0;		// use _DummyBuffer
	callback_ = print_stdout;
    } else {
	if (logsize_b < C7_MLOG_SIZE_MIN || logsize_b > C7_MLOG_SIZE_MAX) {
	    return c7result_err(EINVAL, "invalid logsize_b: %{}", logsize_b);
	}
	auto res = c7::file::mmap_rw(path, mmapsize_b_, true);
	if (!res) {
	    return res;
	}
	top = res.value().release();
	callback_ = nullptr;
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
	if (hint_op != nullptr) {
	    (void)std::strncat(hdr_->hint, hint_op, sizeof(hdr_->hint) - 1);
	}
    }
    if (hdr_->cnt == 0 && hdr_->nextaddr == 0) {
	clear();
    }
}

void
mlog_writer::impl::free_storage()
{
    if (hdr_ != nullptr && mmapsize_b_ != 0) {
	::munmap(hdr_, mmapsize_b_);
    }
    hdr_ = nullptr;
}


// logging --------------------------------------------------------

rec_t
mlog_writer::impl::make_rechdr(size_t logsize, size_t tn_size, size_t sn_size, int src_line,
			       c7::usec_t time_us, uint32_t level, uint32_t category,
			       uint64_t minidata)
{
    rec_t rec;
    
    // null character is not counted for *_size, but it's put to rbuf.
    if (tn_size > 0) {
	logsize += (tn_size + 1);
    }
    if (sn_size > 0) {
	logsize += (sn_size + 1);
    }
    logsize += sizeof(raddr_t);
    logsize += sizeof(rec_t);

    rec.size = logsize;
    // rec.order is assigned later
    rec.time_us = time_us;
    rec.pid = pid_;
    rec.th_id = c7::thread::self::id();
    rec.tn_size = tn_size;
    rec.sn_size = sn_size;
    rec.src_line = src_line;
    rec.minidata = minidata;
    rec.level = level;
    rec.category = category;
    rec.control = 0;
    // rec.br_order is assigned later

    return rec;
}

bool
mlog_writer::impl::put(c7::usec_t time_us, const char *src_name, int src_line,
		       uint32_t level, uint32_t category, uint64_t minidata,
		       const void *logaddr, size_t logsize_b)
{
    if (callback_) {
	callback_(time_us, src_name, src_line, level, category,
		  minidata, logaddr, logsize_b);
    }

    // thread name size
    const char *th_name = nullptr;
    if ((flags_ & C7_MLOG_F_THREAD_NAME) != 0) {
	th_name = c7::thread::self::name();
    }
    size_t tn_size = th_name ? strlen(th_name) : 0;
    if (tn_size > _TN_MAX) {
	th_name += (tn_size - _TN_MAX);
	tn_size = _TN_MAX;
    }

    // source name size
    size_t sn_size;
    if ((flags_ & C7_MLOG_F_SOURCE_NAME) == 0 || src_name == nullptr) {
	src_name = nullptr;
	sn_size = 0;
    } else {
	src_name = c7path_name(src_name);
	const char *sfx = c7path_suffix(src_name);
	if ((sn_size = sfx - src_name) > _SN_MAX) {
	    src_name += (sn_size - _SN_MAX);
	    sn_size = _SN_MAX;
	}
    }

    // build record header and calculate size of whole record.
    auto rechdr = make_rechdr(logsize_b, tn_size, sn_size, src_line,
			      time_us, level, category, minidata);

    // check size to be written
    if ((rechdr.size + 32) > hdr_->logsize_b) {		// (C) ensure rechdr.size < hdr_->logsize_b
	return false;					// data size too large
    }

    // lock-free operation: get address of out record and update hdr_->nextaddr.
    raddr_t addr, next;
    do {
	addr = hdr_->nextaddr;
	next = (addr + rechdr.size) % hdr_->logsize_b;
	// addr != next is ensured by above check (C)
    } while (__sync_val_compare_and_swap(&hdr_->nextaddr, addr, next) != addr);

    // lock-free operation: update record sequence number.
    //                    : next rechdr.order is not strict, but we accept it.
    rechdr.order = __sync_add_and_fetch(&hdr_->cnt, 1);
    rechdr.br_order = ~rechdr.order;

    // write whole record: log data -> thread name -> source name	// (A) cf.(B)
    addr = rbuf_.put(addr, sizeof(rechdr), &rechdr);			// record header
    addr = rbuf_.put(addr, logsize_b, logaddr);				// log data
    if (tn_size > 0) {
	addr = rbuf_.put(addr, tn_size+1, th_name);			// +1 : null character
    }
    if (sn_size > 0) {
	char ch = 0;
	addr = rbuf_.put(addr, sn_size, src_name);			// exclude suffix
	addr = rbuf_.put(addr, 1, &ch);
    }
    rbuf_.put(addr, sizeof(rechdr.size), &rechdr.size);			// put size data to tail

    return true;
}


// clear (reset) --------------------------------------------------

void
mlog_writer::impl::clear()
{
    raddr_t addr = 0;
    hdr_->cnt = 0;
    hdr_->nextaddr = rbuf_.put(addr, sizeof(addr), &addr);
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

result<>
mlog_writer::init(const std::string& name,
		  size_t hdrsize_b, size_t logsize_b,
		  uint32_t w_flags, const char *hint)
{
    auto path = c7::path::init_c7spec(name, ".mlog", C7_MLOG_DIR_ENV);
    return pimpl->init(path.c_str(), hdrsize_b, logsize_b, w_flags, hint);
}

void mlog_writer::set_callback(callback_t callback)
{
    return pimpl->set_callback(callback);
}

void mlog_writer::enable_stdout()
{
    set_callback(print_stdout);
}

void *mlog_writer::hdraddr(size_t *hdrsize_b_op)
{
    return pimpl->hdraddr(hdrsize_b_op);
}

void mlog_writer::post_forked()
{
    pimpl->post_forked();
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
    int max_tn_size_ = 0;
    int max_sn_size_ = 0;

    raddr_t find_origin(raddr_t ret_addr,
			size_t maxcount,
			uint32_t order_min,
			c7::usec_t time_us_min,
			std::function<bool(const info_t&)> choice);

public:
    impl() {}

    ~impl() {
	std::free(hdr_);
    }

    result<> load(const std::string& path);

    void scan(size_t maxcount,
	      uint32_t order_min,
	      c7::usec_t time_us_min,
	      std::function<bool(const info_t&)> choice,
	      std::function<bool(const info_t&, void*)> access);

    void *hdraddr(size_t *hdrsize_b_op) {
	if (hdrsize_b_op != nullptr) {
	    *hdrsize_b_op = hdr_->hdrsize_b;
	}
	return (char *)hdr_ + _IHDRSIZE;
    }

    int thread_name_size() {
	return max_tn_size_;
    }

    int source_name_size() {
	return max_sn_size_;
    }

    const char *hint() {
	return hdr_->hint;
    }
};

static void
make_info(mlog_reader::info_t& info, const rec_t& rec, const char *data)
{
    info.thread_id   = rec.th_id;
    info.source_line = rec.src_line;
    info.weak_order  = rec.order;
    info.size_b      = rec.size;
    info.time_us     = rec.time_us;
    info.level       = rec.level;
    info.category    = rec.category;
    info.minidata    = rec.minidata;
    info.pid         = rec.pid;

    // (B) cf.(A)

    info.size_b -= sizeof(rec);
    info.size_b -= sizeof(raddr_t);

    if (rec.sn_size > 0) {
	info.size_b -= (rec.sn_size + 1);
	info.source_name = data + info.size_b;
    } else {
	info.source_name = "";
    }

    if (rec.tn_size > 0) {
	info.size_b -= (rec.tn_size + 1);
	info.thread_name = data + info.size_b;
    } else {
	info.thread_name = "";
    }
}

result<>
mlog_reader::impl::load(const std::string& path)
{
    auto res = c7::file::read(path);
    if (!res) {
	return res;
    }
    void *top = res.value().release();
    hdr_ = static_cast<hdr_t*>(top);
    rbuf_ = rbuffer(static_cast<char*>(top) + _IHDRSIZE + hdr_->hdrsize_b,
		    hdr_->logsize_b);

    return c7result_ok();
}

raddr_t
mlog_reader::impl::find_origin(raddr_t ret_addr,
			       size_t maxcount,
			       uint32_t order_min,
			       c7::usec_t time_us_min,
			       std::function<bool(const info_t&)> choice)
{
    const raddr_t brk_addr = ret_addr - hdr_->logsize_b;
    rec_t rec;
    
    maxcount = std::min<decltype(maxcount)>(hdr_->cnt, maxcount ? maxcount : (-1UL - 1));
    maxcount++;

    for (;;) {
	raddr_t size;
	rbuf_.get(ret_addr - sizeof(size), sizeof(size), &size);
	const raddr_t addr = ret_addr - size;

	if (size == 0 || addr < brk_addr) {
	    break;
	}
	rbuf_.get(addr, sizeof(rec), &rec);
	if (rec.size != size || rec.order != ~rec.br_order) {
	    break;
	}

	max_sn_size_ = std::max(max_sn_size_, static_cast<int>(rec.sn_size));
	max_tn_size_ = std::max(max_tn_size_, static_cast<int>(rec.tn_size));

	dbuf_.clear();
	dbuf_.reserve(size - sizeof(rec));
	rbuf_.get(addr + sizeof(rec), size - sizeof(rec), dbuf_.data());
	make_info(info_, rec, dbuf_.data());

	rec.control &= ~_REC_CONTROL_CHOICE;		// clear choiced flag previously stored
	if (choice(info_)) {
	    rec.control |= _REC_CONTROL_CHOICE;
	    maxcount--;
	}
	rbuf_.put(addr, sizeof(rec), &rec);		// store rec.control

	if (maxcount == 0 ||
	    rec.order < order_min ||
	    rec.time_us < time_us_min) {
	    break;		
	}

	ret_addr = addr;
    }

    return ret_addr;
}

void
mlog_reader::impl::scan(size_t maxcount,
			uint32_t order_min,
			c7::usec_t time_us_min,
			std::function<bool(const info_t&)> choice,
			std::function<bool(const info_t&, void*)> access)
{
    const raddr_t ret_addr = hdr_->nextaddr + hdr_->logsize_b * 2;
    raddr_t addr = find_origin(ret_addr, maxcount, order_min, time_us_min, choice);

    while (addr < ret_addr) {
	rec_t rec;
	rbuf_.get(addr, sizeof(rec), &rec);
	addr += sizeof(rec);				// data address
	size_t data_size = rec.size - sizeof(rec);	// data (log, names) size
	if ((rec.control & _REC_CONTROL_CHOICE) != 0) {
	    dbuf_.clear();
	    dbuf_.reserve(data_size);
	    rbuf_.get(addr, data_size, dbuf_.data());
	    make_info(info_, rec, dbuf_.data());
	    if (!access(info_, dbuf_.data())) {
		break;
	    }
	}
	addr += data_size;				// next address
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

result<>
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
