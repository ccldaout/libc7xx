/*
 * c7thread/spinlock.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1557099975
 */
#ifndef C7_THREAD_SPINLOCK_HPP_LOADED_
#define C7_THREAD_SPINLOCK_HPP_LOADED_
#include <c7common.hpp>


#include <c7defer.hpp>


namespace c7::thread {


class spinlock {
private:
    class impl;
    impl *pimpl;

public:
    spinlock(const spinlock&) = delete;
    spinlock& operator=(const spinlock&) = delete;

    spinlock();
    spinlock(spinlock&& o) = delete;
    spinlock& operator=(spinlock&& o) = delete;
    ~spinlock();

    void _lock();
    bool _trylock();
    void unlock();

    [[nodiscard]]
    c7::defer lock() {
	_lock();
	return c7::defer([this](){ unlock(); });
    }

    [[nodiscard]]
    c7::defer trylock() {
	if (_trylock()) {
	    return c7::defer([this](){ unlock(); });
	}
	return c7::defer();
    }

    void lock_do(std::function<void()> critical) {
	auto defer = lock();
	critical();
    }

    bool trylock_do(std::function<void()> critical) {
	auto defer = trylock();
	if (defer)
	    critical();
	return static_cast<bool>(defer);
    }

    template <typename R>
    R lock_do(std::function<R()> critical) {
	auto defer = lock();
	return critical();
    }

    template <typename R>
    auto trylock_do(std::function<R()> critical) {
	if (auto defer = trylock(); !defer)
	    return std::make_pair(false, R());
	return std::make_pair(true, critical());
    }
};


} // namespace c7::thread


#endif // c7thread/spinlock.hpp
