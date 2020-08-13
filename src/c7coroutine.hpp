/*
 * c7coroutine.hpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __C7_COROUTINE_HPP_LOADED__
#define __C7_COROUTINE_HPP_LOADED__
#include "c7common.hpp"


#include "c7any.hpp"
#include <functional>
#include <ucontext.h>


namespace c7 {


//----------------------------------------------------------------------------
//                                coroutine
//----------------------------------------------------------------------------

class coroutine;

class coroutine {
public:
    enum yield_status {
	SUCCESS, EXIT, ABORT, TYPE_ERR
    };

    template <typename T>
    struct result {
	yield_status status;
	coroutine *from;
	std::remove_reference_t<T> value;

	result(yield_status status, coroutine *from,
	       std::remove_reference_t<T>&& value):
	    status(status), from(from), value(std::move(value)) {
	}

	result(yield_status status, coroutine *from):
	    status(status), from(from) {
	}

	bool is_success() {
	    return (status == SUCCESS);
	}

	bool is_exit() {
	    return (status == EXIT);
	}
    };

private:
    std::function<void()> func_;
    std::unique_ptr<char> stack_;
    ucontext_t context_;
    yield_status yield_status_;
    coroutine* yield_from_;
    c7::anydata yield_data_;

    void setup_context();
    static void entry_point();
    
    void switch_this_from(coroutine *from);

    template <typename RecvData>
    result<RecvData> make_result(coroutine* co) {
	if (co->yield_status_ != SUCCESS)
	    return result<RecvData>(co->yield_status_, co->yield_from_);

	if (co->yield_data_.has<RecvData>())
	    return result<RecvData>(SUCCESS,
				    co->yield_from_,
				    co->yield_data_.move<RecvData>());

	return result<RecvData>(TYPE_ERR, co->yield_from_);
    }

    void exit_with(yield_status);

public:
    coroutine();
    explicit coroutine(size_t stack_b);
    ~coroutine();

    coroutine(const coroutine&) = delete;
    coroutine(const coroutine&&) = delete;
    coroutine& operator=(const coroutine&) = delete;
    coroutine& operator=(const coroutine&&) = delete;

    template <typename ArgType>
    void assign(coroutine& next,
		const std::function<void(coroutine&, result<ArgType>)>& func) {
	func_ = [&]() {
	    auto current = coroutine::self();
	    auto result = make_result<ArgType>(current);
	    func(next, result);
	};
	setup_context();
    }

    template <typename ArgType>
    void assign(coroutine& next,
		std::function<void(coroutine&, result<ArgType>)>&& func) {
	func_ = [&, func = std::move(func)]() {
	    auto current = coroutine::self();
	    auto result = make_result<ArgType>(current);
	    func(next, result);
	};
	setup_context();
    }

    template <typename SendType>
    coroutine& yield(SendType&& data) {
	yield_status_ = SUCCESS;
	yield_data_ = c7::anydata(std::forward<SendType>(data));
	auto current = coroutine::self();
	switch_this_from(current);	// control is switched to this coroutine.
	return *this;
    }

    template <typename SendType>
    coroutine& yield(const SendType& data) {
	yield_status_ = SUCCESS;
	yield_data_ = c7::anydata(data);
	auto current = coroutine::self();
	switch_this_from(current);	// control is switched to this coroutine.
	return *this;
    }

    coroutine& yield() {
	yield_status_ = SUCCESS;
	yield_data_.empty();
	auto current = coroutine::self();
	switch_this_from(current);	// control is switched to this coroutine.
	return *this;
    }

    template <typename RecvType>
    result<RecvType> recv() {
	auto current = coroutine::self();
	return make_result<RecvType>(current);
    }

    static coroutine* self();

    coroutine* from() {
	return yield_from_;
    }

    void exit() {
	exit_with(EXIT);
    }

    void abort() {
	exit_with(ABORT);
    }
};


template <>
struct coroutine::result<void> {
    yield_status status;
    coroutine *from;
    result(yield_status status, coroutine *from):
	status(status), from(from) {
    }
    bool is_success() {
	return (status == SUCCESS);
    }
    bool is_exit() {
	return (status == EXIT);
    }
};

template <>
inline coroutine::result<void> coroutine::make_result<void>(coroutine* co)
{
    if (co->yield_status_ != SUCCESS)
	return result<void>(co->yield_status_, co->yield_from_);

    if (co->yield_data_.is_empty())
	return result<void>(SUCCESS, co->yield_from_);

    return result<void>(TYPE_ERR, co->yield_from_);
}


//----------------------------------------------------------------------------
//                                generator
//----------------------------------------------------------------------------

template <typename T>
class generator {
public:
    class iterator {
    private:
	coroutine::result<T>& result_;
	bool terminated_;

    public:
	typedef ptrdiff_t difference_type;
	typedef T value_type;
	typedef T* pointer;
	typedef T& reference;
	typedef std::input_iterator_tag iterator_categroy;

	iterator(coroutine::result<T>& result, bool terminated):
	    result_(result), terminated_(terminated) {
	}

	bool operator==(const iterator& rhs) const {
	    if (&result_ != &rhs.result_)
		return false;
	    if (terminated_ && rhs.terminated_)
		return true;
	    return false;
	}

	bool operator!=(const iterator& rhs) const {
	    return !operator==(rhs);
	}

	iterator& operator++() {
	    if (!terminated_) {
		result_ = result_.from->yield().template recv<T>();
		if (!result_.is_success())
		    terminated_ = true;
	    }
	    return *this;
	}

	T& operator*() const {
	    if (terminated_)
		throw std::out_of_range("Maybe end iterator.");
	    return result_.value;
	}
    };

private:
    coroutine::result<T> result_;
    std::unique_ptr<coroutine> co_;
    
public:
    enum exit_status {
	END, BROKEN, ERROR
    };

    generator(size_t stack_b,
	      std::function<void()> func):
	result_(coroutine::SUCCESS, nullptr), co_(new coroutine(stack_b)) {
	co_->assign<void>(*coroutine::self(),
			 [&](coroutine&, coroutine::result<void>) { func(); });
	result_ = co_->yield().recv<T>();
    }

    ~generator() {
    }

    static void yield(T&& data) {	// This function msut be called in generator function
	coroutine::self()->from()->yield(std::move(data));
    }

    static void yield(const T& data) {	// This function msut be called in generator function
	coroutine::self()->from()->yield(data);
    }

    iterator begin() noexcept {
	return iterator(result_, !result_.is_success());
    }
    
    iterator end() noexcept {
	return iterator(result_, true);
    }

    exit_status status() {
	if (result_.is_exit())
	    return END;
	if (result_.is_success())
	    return BROKEN;
	return ERROR;
    }
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7coroutine.hpp
