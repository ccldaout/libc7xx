/*
 * c7mlog/reader.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <queue>
#include <c7file.hpp>
#include <c7nseq/reverse.hpp>
#include <c7path.hpp>
#include "c7mlog/private.hpp"


namespace c7::mlog_impl {


using partition_t = partition7_t;
using hdr_t	  = hdr12_t;
using rec_t	  = rec5_t;
using rbuffer	  = rbuffer7;


class mlog_reader12: public mlog_reader::impl {
public:
    mlog_reader12() {}

    ~mlog_reader12() {
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

private:
    hdr_t *hdr_ = nullptr;
    std::vector<rbuffer> rbufs_;
    std::vector<rec_index_t> recs_;

    void prescan(size_t maxcount,
		 uint32_t order_min,
		 c7::usec_t time_us_min,
		 std::function<bool(const info_t&)>& choice);
};


result<>
mlog_reader12::load(const std::string& path)
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
mlog_reader12::scan(size_t maxcount,
		    uint32_t order_min,
		    c7::usec_t time_us_min,
		    std::function<bool(const info_t&)> choice,
		    std::function<bool(const info_t&, void*)> access)
{
    maxcount = std::min<decltype(maxcount)>(hdr_->cnt, maxcount ? maxcount : (-1UL - 1));

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
mlog_reader12::prescan(size_t maxcount,
		       uint32_t order_min,
		       c7::usec_t time_us_min,
		       std::function<bool(const info_t&)>& choice)
{
    std::vector<rec_reader> readers;
    std::priority_queue<rec_desc_t> prioq;
    std::vector<char> dbuf;

    for (size_t i = 0; i < rbufs_.size(); i++) {
	auto& rd = readers.emplace_back(hdr_->log_beg, rbufs_[i], i);
	if (auto desc = rd.get(choice, order_min, time_us_min, dbuf); desc) {
	    prioq.push(desc.value());
	}
    }

    recs_.clear();

    while (maxcount > 0 && !prioq.empty()) {
	auto desc = prioq.top();
	prioq.pop();

	recs_.push_back(desc.idx);
	maxcount--;

	auto& rd = readers[desc.idx.part];
	if (auto desc = rd.get(choice, order_min, time_us_min, dbuf); desc) {
	    prioq.push(desc.value());
	}
    }
}


std::unique_ptr<mlog_reader::impl>
make_mlog_reader12()
{
    return std::make_unique<mlog_reader12>();
}


} // namespace c7::mlog_impl
