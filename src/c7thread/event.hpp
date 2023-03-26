/*
 * c7thread/event.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=192335276
 */
#ifndef C7_THREAD_EVENT_HPP_LOADED__
#define C7_THREAD_EVENT_HPP_LOADED__
#include <c7common.hpp>


#include <c7thread/condvar.hpp>
#include <c7delegate.hpp>


namespace c7::thread {


class event {
private:
    condvar c_;
    bool event_;

    bool wait_unified(c7::usec_t timeout, bool clear);

public:
    c7::delegate<void> on_set;

    event(const event&) = delete;
    event(event&&) = delete;
    event& operator=(const event&) = delete;
    event& operator=(event&&) = delete;

    explicit event(bool init = false);

    bool is_set();
    void set();
    void clear();
    bool wait(c7::usec_t timeout = -1) {
	return wait_unified(timeout, false);
    }
    bool wait_and_clear(c7::usec_t timeout = -1) {
	return wait_unified(timeout, true);
    }
};


} // namespace c7::thread


#endif // c7thread/event.hpp
