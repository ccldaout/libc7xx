/*
 * c7defer.hpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __C7_DEFER_HPP_LOADED__
#define __C7_DEFER_HPP_LOADED__
#include "c7common.hpp"


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

    void operator()() {
	auto f = func_;
	func_ = nullptr;
	f();
    }

    operator bool() const {
	return bool(func_);
    }

    void cancel() {
	func_ = nullptr;
    }

    ~defer() {
	if (func_)
	    func_();
    }
};


} // namespace c7


#endif // c7defer.hpp
