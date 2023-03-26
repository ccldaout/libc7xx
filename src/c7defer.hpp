/*
 * c7defer.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1792162106
 */
#ifndef C7_DEFER_HPP_LOADED__
#define C7_DEFER_HPP_LOADED__
#include <c7common.hpp>


#include <functional>


namespace c7 {


class defer {
private:
    std::function<void()> func_;

public:
    defer(const defer&) = delete;
    defer& operator=(const defer&) = delete;

    defer(defer&& o): func_(std::move(o.func_)) {
	o.func_ = nullptr;
    }

    defer& operator=(defer&& o) {
	if (this != &o) {
	    func_ = std::move(o.func_);
	    o.func_ = nullptr;
	}
	return *this;
    }

    defer() {}

    explicit defer(const std::function<void()>& f): func_(f) {}

    explicit defer(std::function<void()>&& f): func_(std::move(f)) {}

    template <typename Closer, typename Arg1, typename... Args>
    defer(Closer closer, Arg1 arg1, Args... args): func_([=]() { closer(arg1, args...); }) {}

    defer& operator=(const std::function<void()>& f) {
	func_ = f;
	return *this;
    }

    defer& operator=(std::function<void()>&& f) {
	func_ = std::move(f);
	return *this;
    }

    defer& operator+=(const std::function<void()>& f) {
	if (func_) {
	    func_ = [of=std::move(func_), nf=f]() { nf(); of(); };
	} else {
	    func_ = f;
	}
	return *this;
    }

    defer& operator+=(std::function<void()>&& f) {
	if (func_) {
	    func_ = [of=std::move(func_), nf=std::move(f)]() { nf(); of(); };
	} else {
	    func_ = std::move(f);
	}
	return *this;
    }

    void operator()() {
	auto f = func_;
	func_ = nullptr;
	if (f) {
	    f();
	}
    }

    explicit operator bool() const {
	return static_cast<bool>(func_);
    }

    void cancel() {
	func_ = nullptr;
    }

    bool print_as() const {
	return static_cast<bool>(*this);
    }

    ~defer() {
	if (func_) {
	    func_();
	}
    }
};


} // namespace c7


#endif // c7defer.hpp
