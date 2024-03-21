/*
 * c7thread/jobque.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_THREAD_JOBQUE_HPP_LOADED__
#define C7_THREAD_JOBQUE_HPP_LOADED__
#include <c7common.hpp>


#include <c7thread/_queue.hpp>


namespace c7::thread {


template <typename T,
	  template <typename, typename = std::allocator<T>>
	  class Container = std::list>
class jobque: protected queue_base<T, jobque<T, Container>> {
private:
    using base_type = queue_base<T, jobque<T, Container>>;
    friend class queue_base<T, jobque<T, Container>>;

    Container<T> que_;
    size_t uncommitted_jobs_ = 0;

    bool is_empty() const {
	return que_.size() == 0;
    }

    bool is_overflow(size_t) const {
	return false;
    }

    bool is_idle() const {
	return (uncommitted_jobs_ == 0);
    }

    c7::result<> commit_job(size_t) {
	if (uncommitted_jobs_ == 0) {
	    return c7result_err(EINVAL, "Invalid commit: no uncommitted jobs");
	}
	uncommitted_jobs_--;
	return c7result_ok();
    }

    void put_item(T&& item, size_t) {
	uncommitted_jobs_++;
	que_.emplace_back(std::move(item));
    }

    T get_item(size_t& weight) {
	// DON'T decrement uncommitted_jobs_ here.
	auto item = std::move(que_.front());
	que_.pop_front();
	return item;
    }

    void clear_queue() {
	que_.clear();
    }

    auto begin() {
	return que_.begin();
    }

    auto end() {
	return que_.end();
    }

public:
    c7::result<> put(T&& item, c7::usec_t tmo_us = -1) {
	return base_type::put(std::move(item), 1, tmo_us);
    }

    c7::result<T> get(c7::usec_t tmo_us = -1) {
	[[maybe_unused]] size_t weight;
	return base_type::get(weight, tmo_us);
    }

    c7::result<> commit() {
	return base_type::commit(1);
    }

    using base_type::wait_finished;
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
	  template <typename, typename = std::allocator<std::pair<T, size_t>>>
	  class Container = std::list>
class weight_jobque: public queue_base<T, weight_jobque<T, Container>> {
private:
    friend class queue_base<T, weight_jobque<T, Container>>;

    Container<std::pair<T, size_t>> que_;
    size_t uncommitted_ = 0;
    size_t uncommitted_limit_ = -1UL;

    bool is_empty() const {
	return que_.size() == 0;
    }

    bool is_overflow(size_t weight) const {
	return uncommitted_ + weight > uncommitted_limit_;
    }

    bool is_idle() const {
	return (uncommitted_ == 0);
    }

    c7::result<> commit_job(size_t weight) {
	if (uncommitted_ < weight) {
	    return c7result_err(EINVAL,
				"Invalid weight being commited: weight:%{} > uncommitted:%{}",
				weight, uncommitted_);
	}
	uncommitted_ -= weight;
	return c7result_ok();
    }

    void put_item(T&& item, size_t weight) {
	uncommitted_ += weight;
	que_.emplace_back(std::move(item), weight);
    }

    T get_item(size_t& weight) {
	// DON'T decrease uncommitted_ here.
	auto& iw = que_.front();
	auto item = std::move(iw.first);
	weight = iw.second;
	que_.pop_front();
	return item;
    }

    void clear_queue() {
	que_.clear();
	uncommitted_ = 0;
    }

    auto begin() {
	return que_.begin();
    }

    auto end() {
	return que_.end();
    }

public:
    void set_limit(size_t limit) {
	uncommitted_limit_ = limit;
    }

    c7::result<size_t> wait_progress(size_t done_weight, c7::usec_t tmo_us = -1) {
	auto unlock = this->cv_lock();
	auto abstime_p = c7::mktimespec(tmo_us);
	size_t target = uncommitted_ - std::min(uncommitted_, done_weight);
	while ((this->is_alive() || this->is_closing()) && target < uncommitted_) {
	    if (!this->cv_wait(abstime_p)) {
		return c7result_err(ETIMEDOUT);
	    }
	}
	if (this->is_aborted()) {
	    return c7result_err(EPIPE, "queue is aborted.");
	}
	return c7result_ok(uncommitted_);
    }
};


} // namespace c7::thread


#endif // c7thread/jobque.hpp
