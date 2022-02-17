/*
 * c7thread/mutex.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1557099975
 */
#ifndef C7_THREAD_MUTEX_HPP_LOADED__
#define C7_THREAD_MUTEX_HPP_LOADED__
#include <c7common.hpp>


#include <c7defer.hpp>


namespace c7::thread {


class condvar;

class mutex {
private:
    class impl;
    impl *pimpl;
    friend class condvar;

public:
    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;

    explicit mutex(bool recursive = false);
    mutex(mutex&& o) = delete;
    mutex& operator=(mutex&& o) = delete;
    ~mutex();

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


} // namespace c7::hread


#endif // c7thread/mutex.hpp
