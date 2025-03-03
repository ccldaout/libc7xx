/*
 * c7mlog_w.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <sys/mman.h>
#include <c7file.hpp>
#include <c7path.hpp>
#include <c7thread.hpp>
#include "c7mlog_private.hpp"


namespace c7 {


using partition_t = partition7_t;
using hdr_t = hdr7_t;
using rec_t = rec5_t;
using rbuffer = rbuffer7;


/*----------------------------------------------------------------------------
                                  mlog_clear
----------------------------------------------------------------------------*/

c7::result<> mlog_clear(const std::string& name)
{
    c7::file::unique_mmap<> top;
    auto path = c7::path::find_c7spec(name, ".mlog", C7_MLOG_DIR_ENV);
    if (auto res = c7::file::mmap_rw(path); !res) {
	return res.as_error();
    } else {
	top = std::move(res.value());
    }

    uint32_t rev = *static_cast<uint32_t*>(top.get());
    if (rev >= 7) {
	hdr7_t *h = static_cast<hdr7_t*>(top.get());
	h->cnt = 0;
    } else {
	hdr6_t *h = static_cast<hdr6_t*>(top.get());
	h->cnt = 0;
    }

    return c7result_ok();
}


/*----------------------------------------------------------------------------
                              mlog_writer::impl
----------------------------------------------------------------------------*/

alignas(8) static char _DummyBuffer[_IHDRSIZE + _DUMMY_LOG_SIZE];

class mlog_writer::impl {
private:
    callback_t callback_;
    rbuffer rbufobj_[_PART_CNT];
    rbuffer *rbuf_[_PART_CNT];
    hdr_t *hdr_ = nullptr;
    size_t mmapsize_b_ = 0;
    uint32_t flags_;
    uint32_t pid_;

private:
    result<> setup_storage(const char *path,
			   size_t hdrsize_b,
			   const std::vector<size_t>& size_b_v);
    void setup_context(const char *hint_op,
		       size_t hdrsize_b,
		       const std::vector<size_t>& size_b_v);
		       
    void free_storage();

    rec_t make_rechdr(size_t logsize, size_t tn_size, size_t sn_size, int src_line,
		      c7::usec_t time_us, uint32_t level, uint32_t category,
		      uint64_t minidata);

public:
    impl() {
	for (auto& rbp: rbuf_) {
	    rbp = &rbufobj_[0];
	}
	init_default();
    }

    ~impl() {
	free_storage();
    }

    void init_default();

    result<> init(const char *path, size_t hdrsize_b,
		  std::vector<size_t> size_b_v,
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

void
mlog_writer::impl::init_default()
{
    std::vector<size_t> size_v(_PART_CNT);
    size_v[0]   = _DUMMY_LOG_SIZE;
    hdr_        = reinterpret_cast<hdr_t*>(_DummyBuffer);
    mmapsize_b_ = 0;
    callback_   = print_stdout;
    setup_context(nullptr, 0, size_v);
}

result<>
mlog_writer::impl::init(const char *path,
			size_t hdrsize_b,
			std::vector<size_t> size_b_v,
			uint32_t w_flags, const char *hint)
{
    if (auto res = setup_storage(path, hdrsize_b, size_b_v); !res) {
	init_default();
	return res;
    }

    setup_context(hint, hdrsize_b, size_b_v);
    flags_ = w_flags;
    pid_   = getpid();
    return c7result_ok();
}

result<>
mlog_writer::impl::setup_storage(const char *path,
				 size_t hdrsize_b,
				 const std::vector<size_t>& size_b_v)
{
    free_storage();	// unmap previous map

    if (size_b_v[0] == 0) {
	return c7result_err(EINVAL, "size_b_v[0] must not be zero");
    }
    if (size_b_v.size() != _PART_CNT) {
	return c7result_err(EINVAL, "size_b_v must have %{} items", _PART_CNT);
    }

    mmapsize_b_ = _IHDRSIZE + hdrsize_b;
    for (auto part_size: size_b_v) {
	if (part_size == 0) {
	    continue;
	}
	if (part_size < C7_MLOG_SIZE_MIN || part_size > C7_MLOG_SIZE_MAX) {
	    return c7result_err(EINVAL, "invalid partition size_b: %{}", part_size);
	}
	mmapsize_b_ += part_size;
    }

    void *top;
    if (auto res = c7::file::mmap_rw(path, mmapsize_b_, true); !res) {
	return res.as_error();
    } else {
	top = res.value().release();
    }
    hdr_ = static_cast<hdr_t*>(top);

    callback_ = nullptr;

    return c7result_ok();
}

void
mlog_writer::impl::setup_context(const char *hint_op,
				 size_t hdrsize_b,
				 const std::vector<size_t>& size_b_v)
{
    // setup header

    if (hdr_->rev            != _REVISION   ||
	hdr_->hdrsize_b      != hdrsize_b   ||
	hdr_->part[0].size_b != size_b_v[0] ||
	hdr_->part[1].size_b != size_b_v[1] ||
	hdr_->part[2].size_b != size_b_v[2] ||
	hdr_->part[3].size_b != size_b_v[3] ||
	hdr_->part[4].size_b != size_b_v[4] ||
	hdr_->part[5].size_b != size_b_v[5] ||
	hdr_->part[6].size_b != size_b_v[6] ||
	hdr_->part[7].size_b != size_b_v[7]) {
	(void)std::memset(hdr_, 0, mmapsize_b_);
	hdr_->rev            = _REVISION;
	hdr_->hdrsize_b      = hdrsize_b;
	hdr_->part[0].size_b = size_b_v[0];
	hdr_->part[1].size_b = size_b_v[1];
	hdr_->part[2].size_b = size_b_v[2];
	hdr_->part[3].size_b = size_b_v[3];
	hdr_->part[4].size_b = size_b_v[4];
	hdr_->part[5].size_b = size_b_v[5];
	hdr_->part[6].size_b = size_b_v[6];
	hdr_->part[7].size_b = size_b_v[7];
	if (hint_op != nullptr) {
	    (void)std::strncat(hdr_->hint, hint_op, sizeof(hdr_->hint) - 1);
	}
    }

    // setup partition buffers

    uint64_t off = _IHDRSIZE + hdrsize_b;
    rbuffer *rb  = nullptr;
    for (decltype(_PART_CNT) i = 0; i < _PART_CNT; i++) {
	if (hdr_->part[i].size_b > 0) {
	    rbufobj_[i] = rbuffer(hdr_, off, &hdr_->part[i]);
	    rb   = &rbufobj_[i];
	    off += hdr_->part[i].size_b;
	}
	rbuf_[i] = rb;
    }

    if (hdr_->cnt == 0) {
	clear();
    }
}

void
mlog_writer::impl::free_storage()
{
    if (hdr_ != nullptr && hdr_ != static_cast<void*>(_DummyBuffer)) {
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

    // lock-free operation: reserve space of output record
    auto const rbuf = rbuf_[level];
    raddr_t addr = rbuf->reserve(rechdr.size);
    if (addr == _TOO_LARGE) {
	return false;
    }

    // lock-free operation: update record sequence number.
    //                    : updated rechdr.order is not strict, but we accept it.
    rechdr.order = __sync_add_and_fetch(&hdr_->cnt, 1);
    rechdr.br_order = ~rechdr.order;

    // write whole record: log data -> thread name -> source name	// (A) cf.(B)
    addr = rbuf->put(addr, sizeof(rechdr), &rechdr);			// record header
    addr = rbuf->put(addr, logsize_b, logaddr);				// log data
    if (tn_size > 0) {
	addr = rbuf->put(addr, tn_size+1, th_name);			// +1 : null character
    }
    if (sn_size > 0) {
	char ch = 0;
	addr = rbuf->put(addr, sn_size, src_name);			// exclude suffix
	addr = rbuf->put(addr, 1, &ch);
    }
    rbuf->put(addr, sizeof(rechdr.size), &rechdr.size);			// put size data to tail

    return true;
}


// clear (reset) --------------------------------------------------

void
mlog_writer::impl::clear()
{
    hdr_->cnt = 0;
    for (auto rbp: rbuf_) {
	rbp->clear();
    }
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

    std::vector<size_t> size_b_v(_PART_CNT);
    size_b_v[C7_LOG_MIN] = logsize_b;

    return pimpl->init(path.c_str(), hdrsize_b, size_b_v, w_flags, hint);
}

result<>
mlog_writer::init(const std::string& name,
		  size_t hdrsize_b, std::vector<size_t> logsize_b_v,
		  uint32_t w_flags, const char *hint)
{
    auto path = c7::path::init_c7spec(name, ".mlog", C7_MLOG_DIR_ENV);

    logsize_b_v.resize(_PART_CNT);

    return pimpl->init(path.c_str(), hdrsize_b, logsize_b_v, w_flags, hint);
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
----------------------------------------------------------------------------*/


mlog_writer mlog;


} // namespace c7
