/*
 * c7thread/mask.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=192335276
 */
#ifndef C7_THREAD_MASK_HPP_LOADED__
#define C7_THREAD_MASK_HPP_LOADED__
#include <c7common.hpp>


#include <c7thread/condvar.hpp>


namespace c7::thread {


class mask {
private:
    condvar c_;
    uint64_t mask_;

public:
    mask(const mask&) = delete;
    mask(mask&&) = delete;
    mask& operator=(const mask&) = delete;
    mask& operator=(mask&&) = delete;

    explicit mask(uint64_t ini_mask);

    uint64_t get();
    void clear();
    void on(uint64_t on_mask);
    void off(uint64_t off_mask);
    bool change(std::function<void(uint64_t& in_out)> func);
    uint64_t wait(std::function<uint64_t(uint64_t& in_out)> func, c7::usec_t timeout = -1);

    void change(uint64_t on_mask, uint64_t off_mask) {
	change([on_mask,off_mask](uint64_t& in_out){
		in_out = (in_out|on_mask) & ~off_mask;
	    });
    }

    uint64_t wait_all(uint64_t expect_all, uint64_t clear, c7::usec_t timeout = -1) {
	return wait([expect_all,clear](uint64_t& in_out){
		if ((expect_all & in_out) == expect_all) {
		    in_out &= ~clear;
		    return expect_all;
		}
		return 0UL;
	    }, timeout);
    }

    uint64_t wait_any(uint64_t expect_any, uint64_t clear, c7::usec_t timeout = -1) {
	return wait([expect_any,clear](uint64_t& in_out){
		uint64_t ret = (expect_any & in_out);
		if (ret != 0)
		    in_out &= ~clear;
		return ret;
	    }, timeout);
    }
};


} // namespace c7::thread


#endif // c7thread/mask.hpp
