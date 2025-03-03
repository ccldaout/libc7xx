/*
 * c7mlog_r6.cpp
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
#include "c7mlog_private.hpp"


#undef  _REVISION
#define _REVISION		(6)


namespace c7 {


class rbuffer6 {
private:
    char *top_ = nullptr;
    char *end_ = nullptr;
    raddr_t size_ = 0;

public:
    rbuffer6() {}

    explicit rbuffer6(void *addr, raddr_t size):
	top_(static_cast<char*>(addr)), end_(top_ + size), size_(size) {
    }

    void get(raddr_t addr, raddr_t size, void *_ubuf) {
	char *ubuf = static_cast<char*>(_ubuf);
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

    raddr_t put(raddr_t addr, raddr_t size, const void *_ubuf) {
	const raddr_t ret_addr = addr + size;

	const char *ubuf = static_cast<const char*>(_ubuf);
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


using hdr_t	= hdr6_t;
using rec_t	= rec5_t;
using rbuffer	= rbuffer6;


/*----------------------------------------------------------------------------
                                 mlog_reader6
----------------------------------------------------------------------------*/

class mlog_reader6: public mlog_reader::impl {
private:
    rbuffer rbuf_;
    hdr_t *hdr_ = nullptr;
    std::vector<char> dbuf_;
    info_t info_;

    raddr_t find_origin(raddr_t ret_addr,
			size_t maxcount,
			uint32_t order_min,
			c7::usec_t time_us_min,
			std::function<bool(const info_t&)> choice);

public:
    mlog_reader6() {}

    ~mlog_reader6() {
	std::free(hdr_);
    }

    result<> load(const std::string& path) override;

    void scan(size_t maxcount,
	      uint32_t order_min,
	      c7::usec_t time_us_min,
	      std::function<bool(const info_t&)> choice,
	      std::function<bool(const info_t&, void*)> access) override;

    void *hdraddr(size_t *hdrsize_b_op) override {
	if (hdrsize_b_op != nullptr) {
	    *hdrsize_b_op = hdr_->hdrsize_b;
	}
	return (char *)hdr_ + _IHDRSIZE;
    }

    const char *hint() override {
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
	info.source_name = std::string_view{data + info.size_b, rec.sn_size};
    } else {
	info.source_name = "";
    }

    if (rec.tn_size > 0) {
	info.size_b -= (rec.tn_size + 1);
	info.thread_name = std::string_view{data + info.size_b, rec.tn_size};
    } else {
	info.thread_name = "";
    }
}

result<>
mlog_reader6::load(const std::string& path)
{
    auto res = c7::file::read(path);
    if (!res) {
	return res.as_error();
    }
    void *top = res.value().release();
    hdr_ = static_cast<hdr_t*>(top);
    rbuf_ = rbuffer(static_cast<char*>(top) + _IHDRSIZE + hdr_->hdrsize_b,
		    hdr_->logsize_b);

    return c7result_ok();
}

raddr_t
mlog_reader6::find_origin(raddr_t ret_addr,
			  size_t maxcount,
			  uint32_t order_min,
			  c7::usec_t time_us_min,
			  std::function<bool(const info_t&)> choice)
{
    const raddr_t brk_addr = ret_addr - hdr_->logsize_b;
    rec_t rec;

    maxcount = std::min<decltype(maxcount)>(hdr_->cnt, maxcount ? maxcount : (-1UL - 1));

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
mlog_reader6::scan(size_t maxcount,
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


std::unique_ptr<mlog_reader::impl>
make_mlog_reader6()
{
    return std::make_unique<mlog_reader6>();
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7
