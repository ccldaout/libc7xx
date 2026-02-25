/*
 * c7mlog/reader.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <c7file.hpp>
#include <c7path.hpp>
#include "c7mlog/private.hpp"


namespace c7 {


namespace mlog_impl {


/*----------------------------------------------------------------------------
                                   rbuffer7
----------------------------------------------------------------------------*/

rbuffer7::rbuffer7(void *headaddr, uint64_t off, partition7_t *part):
    part_(part),
    top_(static_cast<char*>(headaddr) + off),
    end_(top_ + part->size_b),
    size_(part->size_b)
{
}

void rbuffer7::clear()
{
    if (size_ > 0) {
	raddr_t addr = 0;
	part_->nextaddr = put(addr, sizeof(addr), &addr);
    }
}

raddr_t rbuffer7::reserve(raddr_t size_b)
{
    // check size to be written
    if ((size_b + 32) > size_) {	// (C) ensure rechdr.size < hdr_->logsize_b
	return _TOO_LARGE;		// data size too large
    }

    volatile raddr_t * const nextaddr_p = &part_->nextaddr;
    raddr_t addr, next;
    do {
	addr = *nextaddr_p;
	next = (addr + size_b) % size_;
	// addr != next is ensured by above check (C)
    } while (__sync_val_compare_and_swap(nextaddr_p, addr, next) != addr);
    return addr;
}

void rbuffer7::get(raddr_t addr, raddr_t size, void *_ubuf)
{
    char *ubuf = static_cast<char*>(_ubuf);
    char *rbuf = top_ + (addr % size_);
    raddr_t rrest = end_ - rbuf;

    while (size > 0) {
	raddr_t cpsize = std::min(size, rrest);

	std::memcpy(ubuf, rbuf, cpsize);
	ubuf += cpsize;
	size -= cpsize;

	rbuf = top_;
	rrest = size_;
    }
}

raddr_t rbuffer7::put(raddr_t addr, raddr_t size, const void *_ubuf)
{
    const raddr_t ret_addr = addr + size;

    const char *ubuf = static_cast<const char*>(_ubuf);
    char *rbuf = top_ + (addr % size_);
    raddr_t rrest = end_ - rbuf;

    while (size > 0) {
	raddr_t cpsize = (size < rrest) ? size : rrest;

	std::memcpy(rbuf, ubuf, cpsize);
	ubuf += cpsize;
	size -= cpsize;

	rbuf = top_;
	rrest = size_;
    }

    return ret_addr;
}


/*----------------------------------------------------------------------------
                                  rec_reader
----------------------------------------------------------------------------*/

void
make_info(mlog_reader::info_t& info, const rec5_t& rec, const char *data)
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


rec_reader::rec_reader(c7::usec_t log_beg, rbuffer7& rbuf, int part):
    log_beg_(log_beg),
    rbuf_(rbuf),
    part_(part),
    recaddr_(rbuf.nextaddr() + rbuf.size() * 2),
    brkaddr_(recaddr_ - rbuf.size())
{
}


std::optional<rec_desc_t>
rec_reader::get(std::function<bool(const info_t&)>& choice,
		raddr_t order_min,
		c7::usec_t time_us_min,
		std::vector<char>& dbuf)
{
    info_t info;
    rec5_t rec;

    time_us_min = std::max(time_us_min, log_beg_);

    for (;;) {
	raddr_t size;
	rbuf_.get(recaddr_ - sizeof(size), sizeof(size), &size);
	recaddr_ -= size;

	if (size == 0 || recaddr_ < brkaddr_) {
	    return std::optional<rec_desc_t>{};
	}

	rbuf_.get(recaddr_, sizeof(rec), &rec);
	if (rec.size != size || rec.order != ~rec.br_order ||
	    rec.order < order_min || rec.time_us < time_us_min) {
	    return std::optional<rec_desc_t>{};
	}

	auto dsize = size - sizeof(rec);
	dbuf.clear();
	dbuf.reserve(dsize);
	rbuf_.get(recaddr_ + sizeof(rec), dsize, dbuf.data());
	make_info(info, rec, dbuf.data());
	if (choice(info)) {
	    rec_desc_t desc;
	    desc.time_us  = rec.time_us;
	    desc.order    = rec.order;
	    desc.idx.part = part_;
	    desc.idx.addr = recaddr_;
	    desc.sn_size  = rec.sn_size;
	    desc.tn_size  = rec.tn_size;
	    return desc;
	}
    }
}


} // namespace mlog_impl


/*----------------------------------------------------------------------------
                                 mlog_reader
----------------------------------------------------------------------------*/

mlog_reader::mlog_reader() = default;

mlog_reader::~mlog_reader() = default;

mlog_reader::mlog_reader(mlog_reader&& o): pimpl(std::move(o.pimpl))
{
}

mlog_reader& mlog_reader::operator=(mlog_reader&& o)
{
    if (this != &o) {
	pimpl = std::move(o.pimpl);
    }
    return *this;
}

result<>
mlog_reader::load(const std::string& name)
{
    auto path = c7::path::find_c7spec(name, mlog_impl::suffix(name), C7_MLOG_DIR_ENV);

    uint32_t rev;
    if (auto res = c7::file::read_into(path, rev); !res) {
	return res.as_error();
    } else if (res.value() != 0) {
	return c7result_err(EINVAL, "Invalid mlog format");
    }
    if (rev < 6) {
	pimpl = mlog_impl::make_mlog_reader6();
    } else if (rev == 7) {
	pimpl = mlog_impl::make_mlog_reader7();
    } else {
	pimpl = mlog_impl::make_mlog_reader12();
    }

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

const char *mlog_reader::hint()
{
    return pimpl->hint();
}


} // namespace c7
