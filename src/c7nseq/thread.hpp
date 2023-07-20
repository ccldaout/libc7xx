/*
 * c7nseq/thread.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ_THREAD_HPP_LOADED__
#define C7_NSEQ_THREAD_HPP_LOADED__


#include <list>
#include <vector>
#include <c7thread/queue.hpp>
#include <c7thread/thread.hpp>
#include <c7nseq/_cmn.hpp>
#include <c7nseq/empty.hpp>


namespace c7::nseq {


namespace {

template <typename T>
class double_buffer {

private:
    size_t size_;
    std::list<std::vector<T>> free_;
    std::list<std::vector<T>> wait_;
    c7::thread::condvar cv_;

public:
    explicit double_buffer(size_t size): size_(size) {
	std::vector<T> v1, v2;
	v1.reserve(size_);
	v2.reserve(size_);
	free_.push_back(std::move(v1));
	free_.push_back(std::move(v2));
    }

    // for producer

    void init_producer(std::vector<T>& buf) {
	auto unlock = cv_.lock();
	buf = std::move(free_.front());
	free_.pop_front();
    }

    bool put(std::vector<T>& buf) {
	auto unlock = cv_.lock();
	wait_.push_back(std::move(buf));
	cv_.notify_all();
	while (free_.empty()) {
	    cv_.wait();
	}
	buf = std::move(free_.front());
	buf.clear();
	free_.pop_front();
	return (buf.capacity() == size_);
    }

    void flush(std::vector<T>& buf) {
	auto unlock = cv_.lock();
	wait_.push_back(std::move(buf));
	cv_.notify_all();
    }

    void shutdown_producer() {
	std::vector<T> empty;
	auto unlock = cv_.lock();
	wait_.push_back(std::move(empty));
	cv_.notify_all();
    }

    // for consumer

    void init_consumer(std::vector<T>& buf) {
	auto unlock = cv_.lock();
	while (wait_.empty()) {
	    cv_.wait();
	}
	buf = std::move(wait_.front());
	wait_.pop_front();
    }

    bool get(std::vector<T>& buf) {
	auto unlock = cv_.lock();
	free_.push_back(std::move(buf));
	cv_.notify_all();
	while (wait_.empty()) {
	    cv_.wait();
	}
	buf = std::move(wait_.front());
	wait_.pop_front();
	return (buf.size() != 0);
    }

    void shutdown_consumer() {
	std::vector<T> empty;
	auto unlock = cv_.lock();
	free_.push_back(empty);
	cv_.notify_all();
    }
};

}


// thread

struct thread_iter_end {};


template <typename T>
class th_output {
private:
    size_t buffer_size_;
    double_buffer<T> dbuf_;
    std::vector<T> v_th_;

public:
    explicit th_output(size_t buffer_size):
	buffer_size_(buffer_size), dbuf_(buffer_size) {
	dbuf_.init_producer(v_th_);
    }

    // for thread (producer)

    auto& operator<<(const T& data) {
	if (v_th_.size() == v_th_.capacity()) {
	    if (!dbuf_.put(v_th_)) {
		c7::thread::self::exit();
	    }
	}
	v_th_.push_back(data);
	return *this;
    }

    auto& operator<<(T&& data) {
	if (v_th_.size() == v_th_.capacity()) {
	    if (!dbuf_.put(v_th_)) {
		c7::thread::self::exit();
	    }
	}
	v_th_.push_back(std::move(data));
	return *this;
    }

    template <typename Seq,
	      typename = std::enable_if_t<c7::typefunc::is_sequence_of_v<Seq, T>>>
    auto& operator<<(Seq&& seq) {
	for (auto&& v: seq) {
	    this->operator<<(std::forward<decltype(v)>(v));
	}
	return *this;
    }

    void flush() {
	if (v_th_.size() != 0) {
	    dbuf_.flush(v_th_);
	}
    }

    void shutdown_producer() {
	dbuf_.shutdown_producer();
    }

    // for iterator (consumer)

    void init_consumer(std::vector<T>& buffer) {
	dbuf_.init_consumer(buffer);
    }

    void get(std::vector<T>& buffer) {
	dbuf_.get(buffer);
    }

    void shutdown_consumer() {
	dbuf_.shutdown_consumer();
    }
};


template <typename Seq, typename Output, typename Thread>
class thread_iter {
public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= c7::typefunc::remove_cref_t<Output>;
    using pointer		= value_type*;
    using reference		= value_type&;

private:
    struct context {
	Seq& seq_;
	th_output<value_type> out_;
	c7::thread::thread th_;

	context(Seq& seq, Thread func, size_t buffer_size): seq_(seq), out_(buffer_size) {
	    th_.target(
		[this, func=std::move(func)]() mutable {
		    if constexpr (std::is_invocable_v<Thread, Seq&, decltype(out_)&>) {
			func(seq_, out_);
		    } else {
			func(out_);
		    }
		    out_.flush();
		});
	    th_.finalize(
		[this](){
		    out_.shutdown_producer();
		});
	    th_.start().check();
	}

	void join() {
	    th_.join();
	    th_.terminate_result().check();
	}

	~context() {
	    out_.shutdown_consumer();
	    th_.join();
	}

    };

    std::shared_ptr<context> ctx_;	// for copyable
    std::vector<value_type> vec_;
    size_t idx_ = 0;

public:
    thread_iter() = default;
    thread_iter(const thread_iter&) = default;
    thread_iter& operator=(const thread_iter&) = default;
    thread_iter(thread_iter&&) = default;
    thread_iter& operator=(thread_iter&&) = default;

    thread_iter(Seq& seq, Thread func, size_t buffer_size):
	ctx_(std::make_shared<context>(seq, func, buffer_size)) {
	ctx_->out_.init_consumer(vec_);
    }

    bool operator==(const thread_iter& o) const {
	return ctx_ == o.ctx_;
    }

    bool operator!=(const thread_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const thread_iter_end&) const {
	if (vec_.size() != 0) {
	    return false;
	}
	ctx_->join();
	return true;
    }

    bool operator!=(const thread_iter_end& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	if (idx_ == vec_.size() - 1) {
	    if (vec_.size() != 0) {
		ctx_->out_.get(vec_);
		idx_ = 0;
	    }
	} else {
	    idx_++;
	}
	return *this;
    }

    decltype(auto) operator*() {
	return std::move(vec_[idx_]);
    }
};


template <typename Seq, typename Output, typename Thread>
class thread_obj {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    Thread func_;
    size_t buffer_size_;

public:
    thread_obj(Seq seq, Thread func, size_t buffer_size):
	seq_(std::forward<Seq>(seq)), func_(func), buffer_size_(buffer_size) {
    }

    thread_obj(const thread_obj&) = delete;
    thread_obj& operator=(const thread_obj&) = delete;
    thread_obj(thread_obj&&) = default;
    thread_obj& operator=(thread_obj&&) = delete;

    auto begin() {
	return thread_iter<hold_type, Output, Thread>(seq_, func_, buffer_size_);
    }

    auto end() {
	return thread_iter_end{};
    }

    auto begin() const {
	return const_cast<thread_obj<Seq, Output, Thread>*>(this)->begin();
    }

    auto end() const {
	return thread_iter_end{};
    }
};


template <typename Output, typename Thread>
class thread_seq {
public:
    explicit thread_seq(Thread func, size_t buffer_size):
	func_(func), buffer_size_(buffer_size) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return thread_obj<decltype(seq), Output, Thread>
	    (std::forward<Seq>(seq), func_, buffer_size_);
    }

    auto operator()() {
	empty_seq<> seq;
	return thread_obj<decltype(seq), Output, Thread>
	    (seq, func_, buffer_size_);
    }

private:
    Thread func_;
    size_t buffer_size_;
};


template <typename Output, typename Thread>
auto thread(Thread func, size_t buffer_size=8192)
{
    return thread_seq<Output, Thread>(func, buffer_size);
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename Seq, typename Output, typename Thread>
struct format_ident<c7::nseq::thread_obj<Seq, Output, Thread>> {
    static constexpr const char *name = "thread";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/thread.hpp
