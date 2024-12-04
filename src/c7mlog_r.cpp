/*
 * c7mlog_r.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <optional>
#include <queue>
#include <c7file.hpp>
#include <c7nseq/reverse.hpp>
#include <c7path.hpp>
#include "c7mlog_private.hpp"


namespace c7 {


struct rec_index_t {
    int part;
    raddr_t addr;
};


struct rec_desc_t {
    c7::usec_t time_us;
    uint32_t order;
    rec_index_t idx;
    int tn_size;
    int sn_size;
};


class rec_reader {
public:
    using info_t = mlog_reader::info_t;

    rec_reader(rbuffer& rbuf, int part);
    std::optional<rec_desc_t> get(std::function<bool(const info_t&)>& choice,
				  raddr_t order_min,
				  c7::usec_t time_us_min,
				  std::vector<char>& dbuf);

private:
    rbuffer& rbuf_;
    int part_;
    raddr_t recaddr_;
    raddr_t brkaddr_;
};


static bool operator<(const rec_desc_t& a, const rec_desc_t& b)
{
    return (a.time_us < b.time_us ||
	    (a.time_us == b.time_us && a.order < b.order));
}


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


rec_reader::rec_reader(rbuffer& rbuf, int part):
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
    rec_t rec;

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


/*----------------------------------------------------------------------------
                              mlog_reader::impl
----------------------------------------------------------------------------*/

class mlog_reader::impl {
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

private:
    hdr_t *hdr_ = nullptr;
    std::vector<rbuffer> rbufs_;

    std::vector<rec_index_t> recs_;
    int max_tn_size_ = 0;
    int max_sn_size_ = 0;

    void prescan(size_t maxcount,
		 uint32_t order_min,
		 c7::usec_t time_us_min,
		 std::function<bool(const info_t&)>& choice);
};


result<>
mlog_reader::impl::load(const std::string& path)
{
    size_t size_b = 0;
    auto res = c7::file::read(path, size_b);
    if (!res) {
	return res.as_error();
    }
    void *top = res.value().release();
    hdr_ = static_cast<hdr_t*>(top);

    if (size_b < _IHDRSIZE) {
	return c7result_err(EINVAL, "Too small: no space for header");
    }
    if (hdr_->rev != _REVISION) {
	return c7result_err(EINVAL, "Revision mismatch: header:%{}, library:%{}",
			    hdr_->rev, _REVISION);
    }
    uint64_t reqsize_b = _IHDRSIZE + hdr_->hdrsize_b;
    for (decltype(_PART_CNT) i = 0; i < _PART_CNT; i++) {
	reqsize_b += hdr_->part[i].size_b;
    }
    if (size_b < reqsize_b) {
	return c7result_err(EINVAL, "Too small: require size from part[*]: %{}", reqsize_b);
    }

    uint64_t off = _IHDRSIZE + hdr_->hdrsize_b;
    for (decltype(_PART_CNT) i = 0; i < _PART_CNT; i++) {
	if (hdr_->part[i].size_b > 0) {
	    rbufs_.emplace_back(hdr_, off, &hdr_->part[i]);
	    off += hdr_->part[i].size_b;
	}
    }

    return c7result_ok();
}


void
mlog_reader::impl::scan(size_t maxcount,
			uint32_t order_min,
			c7::usec_t time_us_min,
			std::function<bool(const info_t&)> choice,
			std::function<bool(const info_t&, void*)> access)
{
    maxcount = std::min<decltype(maxcount)>(hdr_->cnt, maxcount ? maxcount : (-1UL - 1));
    maxcount++;

    prescan(maxcount, order_min, time_us_min, choice);

    std::vector<char> dbuf;
    info_t info;
    rec_t rec;

    for (auto [part, recaddr]: recs_ | c7::nseq::reverse()) {
	auto& rbuf = rbufs_[part];
	rbuf.get(recaddr, sizeof(rec), &rec);

	auto dsize = rec.size - sizeof(rec);
	dbuf.clear();
	dbuf.reserve(dsize);
	rbuf.get(recaddr + sizeof(rec), dsize, dbuf.data());
	make_info(info, rec, dbuf.data());

	if (!access(info, dbuf.data())) {
	    return;
	}
    }
}


void
mlog_reader::impl::prescan(size_t maxcount,
			   uint32_t order_min,
			   c7::usec_t time_us_min,
			   std::function<bool(const info_t&)>& choice)
{
    std::vector<rec_reader> readers;
    std::priority_queue<rec_desc_t> prioq;
    std::vector<char> dbuf;

    for (size_t i = 0; i < rbufs_.size(); i++) {
	auto& rd = readers.emplace_back(rbufs_[i], i);
	if (auto desc = rd.get(choice, order_min, time_us_min, dbuf); desc) {
	    prioq.push(desc.value());
	}
    }

    max_sn_size_ = 0;
    max_tn_size_ = 0;
    recs_.clear();

    while (maxcount > 0 && !prioq.empty()) {
	auto desc = prioq.top();
	prioq.pop();

	recs_.push_back(desc.idx);
	maxcount--;
	max_sn_size_ = std::max(max_sn_size_, desc.sn_size);
	max_tn_size_ = std::max(max_tn_size_, desc.tn_size);

	auto& rd = readers[desc.idx.part];
	if (auto desc = rd.get(choice, order_min, time_us_min, dbuf); desc) {
	    prioq.push(desc.value());
	}
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
