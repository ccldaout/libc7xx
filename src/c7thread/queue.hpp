/*
 * c7thread/queue.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 */
#ifndef C7_THREAD_QUEUE_HPP_LOADED__
#define C7_THREAD_QUEUE_HPP_LOADED__
#include <c7common.hpp>


#include <c7thread/_queue.hpp>


namespace c7::thread {


template <typename T,
	  template <typename, typename = std::allocator<T>>
	  class Container = std::list>
class queue: protected queue_base<T, queue<T, Container>> {
private:
    using base_type = queue_base<T, queue<T, Container>>;
    friend class queue_base<T, queue<T, Container>>;

    Container<T> que_;
    size_t limit_ = -1UL;

    bool is_empty() const {
	return que_.size() == 0;
    }

    bool is_overflow(size_t) const {
	return (que_.size() >= limit_);
    }

    bool is_idle() const {
	return que_.size() == 0;
    }

    void put_item(T&& item, size_t) {
	que_.emplace_back(std::move(item));
    }

    T get_item(size_t& weight) {
	auto item = std::move(que_.front());
	que_.pop_front();
	return item;
    }

    void clear_queue() {
	que_.clear();
    }

protected:
    auto begin() {
	return que_.begin();
    }

    auto end() {
	return que_.end();
    }

public:
    void set_limit(size_t limit) {
	limit_ = limit;
    }

    c7::result<> put(T&& item, c7::usec_t tmo_us = -1) {
	return base_type::put(std::move(item), 1, tmo_us);
    }

    c7::result<T> get(c7::usec_t tmo_us = -1) {
	[[maybe_unused]] size_t weight;
	return base_type::get(weight, tmo_us);
    }

    using base_type::close;
    using base_type::abort;
    using base_type::reset;
    using base_type::scan;
    using base_type::is_aborted;
    using base_type::is_closed;
    using base_type::is_closing;
    using base_type::is_alive;
};


template <typename T,
	  template <typename, typename = std::allocator<T>>
	  class Container = std::list>
class sync_queue: public queue<T, Container> {
private:
    using base_type = queue<T, Container>;

    uint32_t sync_ = 0;

public:
    c7::result<> sync_finish(c7::usec_t tmo_us = -1) {
	auto unlock = this->cv_lock();
	sync_++;
	this->cv_notify_all();
	auto abstime_p = c7::mktimespec(tmo_us);
	while (!this->is_aborted() && sync_ != 2) {
	    if (!this->cv_wait(abstime_p)) {
		return c7result_err(ETIMEDOUT);
	    }
	}
	if (this->is_aborted()) {
	    return c7result_err(EPIPE, "queue is aborted.");
	}
	return c7result_ok();
    }
};


} // namespace c7::thread


#endif // c7thread/queue.hpp
