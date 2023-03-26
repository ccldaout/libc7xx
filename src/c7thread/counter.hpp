/*
 * c7thread/counter.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=192335276
 */
#ifndef C7_THREAD_COUNTER_HPP_LOADED__
#define C7_THREAD_COUNTER_HPP_LOADED__
#include <c7common.hpp>


#include <c7thread/condvar.hpp>
#include <c7delegate.hpp>


namespace c7::thread {


class counter {
private:
    condvar c_;
    int counter_;

public:
    c7::delegate<void> on_zero;

    counter(const counter&) = delete;
    counter(counter&&) = delete;
    counter& operator=(const counter&) = delete;
    counter& operator=(counter&&) = delete;

    explicit counter(int init_count = 0);

    int count();
    void reset(int count);
    void up(int n = 1);
    bool down(c7::usec_t timeout = -1);
    bool wait_atleast(int expect, c7::usec_t timeout = -1);
    bool wait_just(int expect, c7::usec_t timeout = -1);
};


} // namespace c7::thread


#endif // c7thread/counter.hpp
