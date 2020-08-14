/*
 * c7any.hpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __C7_ANY_HPP_LOADED__
#define __C7_ANY_HPP_LOADED__
#include <c7common.hpp>


#include <c7typefunc.hpp>
#include <functional>
#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <type_traits>
#include <sys/time.h>


namespace c7 {


/*----------------------------------------------------------------------------
                        anydata (hold data as untyped)
----------------------------------------------------------------------------*/

class anydata {
private:
    class untyped {
    public:
	virtual ~untyped() {}
    };

    template <typename T>
    class typed_delived: public untyped {
    private:
	T data_;

    public:
	explicit typed_delived(T&& d):
	    data_(std::move(d)) {
	}
	explicit typed_delived(const T& d):
	    data_(d) {
	}
	virtual ~typed_delived() {}

	typed_delived(const typed_delived<T>&) = delete;
	void operator=(const typed_delived<T>&) = delete;

	T&& typed() {
	    return std::move(data_);
	}
    };

    template <typename T>
    inline auto delived_ptr(untyped* base) {
	return dynamic_cast<typed_delived<std::remove_reference_t<T>>*>(base);
    }

private:
    std::unique_ptr<untyped> untyped_;

public:
    anydata(const anydata&) = delete;
    anydata& operator=(const anydata&) = delete;

    anydata() {}

    anydata(anydata&& o): untyped_(std::move(o.untyped_)) {}

    template <typename T>
    explicit anydata(T&& d):
	untyped_(
	    new typed_delived<std::remove_reference_t<T>>(std::forward<T>(d))
	    ) {}

    template <typename T>
    explicit anydata(const T& d):
	untyped_(
	    new typed_delived<std::remove_reference_t<T>>(d)
	    ) {}

    anydata& operator=(anydata&& other) {
	if (this != &other) {
	    untyped_ = std::move(other.untyped_);
	}
	return *this;
    }

    template <typename T>
    anydata& operator=(const T& d) {
	untyped_ = new typed_delived<std::remove_reference_t<T>>(d);
	return *this;
    }

    template <typename T>
    anydata& operator=(const T&& d) {
	untyped_ = new typed_delived<std::remove_reference_t<T>>(std::forward<T>(d));
	return *this;
    }

    void empty() {
	untyped_.reset();
    }

    bool is_empty() const {
	return (untyped_.get() == nullptr);
    }

    operator bool() const {
	return !is_empty();
    }

    template <typename T>
    bool has() {
	auto base = untyped_.get();
	return (base != nullptr && delived_ptr<T>(base) != nullptr);
    }

    template <typename T>
    std::remove_reference_t<T>&& move() {
	auto base = untyped_.get();
	if (base == nullptr)
	    throw std::runtime_error("anydata: has no data (maybe has been moved)");
	auto delived = delived_ptr<T>(base);
	if (delived == nullptr)
	    throw std::runtime_error("anydata: dynamic_cast failed: type mismatch");
	return delived->typed();
    }
};


/*----------------------------------------------------------------------------
                      anyfunc (hold callable as untyped)
----------------------------------------------------------------------------*/

template <typename Return = void>
class anyfunc {
private:
    struct hold_base {
	virtual ~hold_base() {}
    };

    template <typename... Args>
    struct hold: public hold_base {
	std::function<Return(Args...)> func_;

	explicit hold(const std::function<Return(Args...)>& func):
	    hold_base(), func_(func) {
	}

	explicit hold(std::function<Return(Args...)>&& func):
	    hold_base(), func_(std::move(func)) {
	}
    };

    std::unique_ptr<hold_base> hold_;

    template <typename... Args>
    auto downcast(hold_base* base) {
	typedef typename c7::typefunc::map_t<c7::typefunc::unwrapref, Args...> unwrap_tuple;
	typedef typename c7::typefunc::apply_t<hold, unwrap_tuple> unwrap_hold;
	return dynamic_cast<unwrap_hold*>(base);
    }

    template <typename... Args>
    auto disclose() {
	auto base = hold_.get();
	if (base == nullptr)
	    throw std::runtime_error("anyfunc: has no function");
	auto h = downcast<Args...>(base);
	if (h == nullptr) 
	    throw std::runtime_error("anyfunc: dynamic_cast failed: type mismatch");
	return h;
    }

public:
    template <typename... Args>
    using args = std::function<Return(Args...)>;

    anyfunc(): hold_(nullptr) {}

    // one or more arugments

    template <typename Arg1, typename... Args>
    anyfunc(const std::function<Return(Arg1, Args...)>& f):
	hold_(new hold<Arg1, Args...>(f)) {
    }

    template <typename Arg1, typename... Args>
    anyfunc(std::function<Return(Arg1, Args...)>&& f):
	hold_(new hold<Arg1, Args...>(std::move(f))) {
    }

    template <typename Arg1, typename... Args>
    anyfunc<Return>& operator=(const std::function<Return(Arg1, Args...)>& f) {
	hold_.reset(new hold<Arg1, Args...>(f));
	return *this;
    }

    template <typename Arg1, typename... Args>
    anyfunc<Return>& operator=(std::function<Return(Arg1, Args...)>&& f) {
	hold_.reset(new hold<Arg1, Args...>(std::move(f)));
	return *this;
    }

    // no argument

    anyfunc(const std::function<Return()>& f):
	hold_(new hold<>(f)) {
    }

    anyfunc(std::function<Return()>&& f):
	hold_(new hold<>(std::move(f))) {
    }

    anyfunc<Return>& operator=(const std::function<Return()>& f) {
	hold_.reset(new hold<>(f));
	return *this;
    }

    anyfunc<Return>& operator=(std::function<Return()>&& f) {
	hold_.reset(new hold<>(std::move(f)));
	return *this;
    }

    // check

    bool is_empty() const {
	return (hold_.get() == nullptr);
    }

    operator bool() const {
	return !is_empty();
    }

    template <typename... Args>
    bool has() {
	auto base = hold_.get();
	return (base != nullptr && downcast<Args...>(base) != nullptr);
    }

    // call

    template <typename... Args>
    Return call(Args... args) {
	auto h = disclose<Args...>();
	return h->func_(args...);
    }

    template <typename... Args>
    Return operator()(Args... args) {
	return call(args...);
    }
};

template <>
template <typename... Args>
void anyfunc<void>::call(Args... args)
{
    auto h = disclose<Args...>();
    h->func_(args...);
};

template <>
template <typename... Args>
void anyfunc<void>::operator()(Args... args)
{
    call(args...);
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7any.hpp
