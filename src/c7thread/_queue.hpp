/*
 * c7thread/_queue.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 */
#ifndef C7_THREAD__QUEUE_HPP_LOADED__
#define C7_THREAD__QUEUE_HPP_LOADED__
#include <c7common.hpp>


#include <c7result.hpp>
#include <c7utils.hpp>
#include <c7thread/condvar.hpp>


namespace c7::thread {


template <typename T, typename Target>
class queue_base {
private:
    enum run_status { ALIVE, CLOSING, CLOSED, ABORTED };
    condvar cv_;
    run_status status_ = ALIVE;

    bool is_empty() const {
	return static_cast<const Target*>(this)->is_empty();
    }
    bool is_overflow(size_t weight) const {
	return static_cast<const Target*>(this)->is_overflow(weight);
    }
    bool is_idle() const {	// all items are committed [also queue is empty]
	return static_cast<const Target*>(this)->is_idle();
    }
    c7::result<> commit_job(size_t weight) {
	return static_cast<Target*>(this)->commit_job(weight);
    }
    void put_item(T&& item, size_t weight) {
	return static_cast<Target*>(this)->put_item(std::move(item), weight);
    }
    T get_item(size_t& weight) {
	return static_cast<Target*>(this)->get_item(weight);
    }
    void clear_queue() {
	return static_cast<Target*>(this)->clear_queue();
    }

    auto begin() {
	return static_cast<Target*>(this)->begin();
    }

    auto end() {
	return static_cast<Target*>(this)->end();
    }

protected:
    c7::defer cv_lock() {
	return cv_.lock();
    }

    bool cv_wait(const ::timespec* timeout_abs) {
	return cv_.wait(timeout_abs);
    }

    void cv_notify_all() {
	cv_.notify_all();
    }

public:
    c7::result<> put(T&& item, size_t weight, c7::usec_t tmo_us = -1) {
	auto unlock = cv_.lock();
	auto abstime_p = c7::mktimespec(tmo_us);
	while (is_alive() && is_overflow(weight)) {
	    if (!cv_.wait(abstime_p)) {
		return c7result_err(ETIMEDOUT);
	    }
	}
	if (is_alive()) {
	    put_item(std::move(item), weight);
	    cv_.notify_all();
	    return c7result_ok();
	} else {
	    return c7result_err(EPIPE, "queue is closed or aborted");
	}
    }

    c7::result<T> get(size_t& weight, c7::usec_t tmo_us = -1) {
	weight = 0;
	auto unlock = cv_.lock();
	auto abstime_p = c7::mktimespec(tmo_us);
	while (is_alive() && is_empty()) {
	    if (!cv_.wait(abstime_p)) {
		return c7result_err(ETIMEDOUT);
	    }
	}
	if (is_closed()) {
	    return c7result_err(ENODATA, "queue is closed");
	}
	if (is_empty()) {
	    return c7result_err(EPIPE, "queue is aborted");
	}
	auto item = get_item(weight);
	cv_.notify_all();
	return c7result_ok(std::move(item));
    }

    c7::result<> commit(size_t weight) {
	auto unlock = cv_.lock();
	if (auto res = commit_job(weight); !res) {
	    return res;
	}
	if (is_closing() && is_idle()) {
	    status_ = CLOSED;
	}
	cv_.notify_all();
	return c7result_ok();
    }

    c7::result<> wait_finished(c7::usec_t tmo_us = -1) {
	// [CAUTION] wait_finished() work with commit() operation
	auto unlock = this->cv_lock();
	auto abstime_p = c7::mktimespec(tmo_us);
	while (this->is_alive() || this->is_closing()) {
	    if (!this->cv_wait(abstime_p)) {
		return c7result_err(ETIMEDOUT);
	    }
	}
	if (this->is_aborted()) {
	    return c7result_err(EPIPE, "queue is aborted.");
	}
	return c7result_ok();
    }

    void close() {
	auto unlock = cv_.lock();
	if (is_idle()) {
	    status_ = CLOSED;
	} else {
	    status_ = CLOSING;
	}
	cv_.notify_all();
    }

    void abort() {
	auto unlock = cv_.lock();
	clear_queue();
	status_ = ABORTED;
	cv_.notify_all();
    }

    void reset() {
	clear_queue();
	status_ = ALIVE;
    }

    template <typename Func>
    c7::result<> scan(Func func) {
	auto unlock = cv_.lock();
	for (auto it = begin(); it != end(); ++it) {
	    if (auto res = func(&*it); !res) {
		return res;
	    }
	}
	return c7result_ok();
    }

    bool is_aborted() const {
	return (status_ == ABORTED);
    }

    bool is_closed() const {
	return (status_ == CLOSED);
    }

    bool is_closing() const {
	return (status_ == CLOSING);
    }

    bool is_alive() const {
	return (status_ == ALIVE);
    }
};


} // namespace c7::thread


#endif // c7thread/_queue.hpp
