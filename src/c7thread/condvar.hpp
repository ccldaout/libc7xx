/*
 * c7thread/condvar.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1557099975
 */
#ifndef C7_THREAD_CONDVAR_HPP_LOADED_
#define C7_THREAD_CONDVAR_HPP_LOADED_
#include <c7common.hpp>


#include <c7defer.hpp>


namespace c7::thread {


class mutex;

class condvar {
private:
    class impl;
    impl *pimpl;

public:
    condvar(const condvar&) = delete;
    condvar& operator=(const condvar&) = delete;

    condvar();
    explicit condvar(mutex& mutex);
    condvar(condvar&&) = delete;
    condvar& operator=(condvar&&) = delete;
    ~condvar();

    void _lock();
    bool _trylock();
    void unlock();

    void wait();
    bool wait(const ::timespec* timeout_abs);		// accept nullptr
    void notify();
    void notify_all();

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

    void wait_for(std::function<bool()> break_pred) {
	while (!break_pred()) {
	    wait();
	}
    }

    bool wait_for(const ::timespec* timeout_abs, std::function<bool()> break_pred) {
	while (!break_pred()) {
	    if (!wait(timeout_abs))
		return false;
	}
	return true;
    }

    void wait_while(std::function<bool()> wait_pred) {
	while (wait_pred()) {
	    wait();
	}
    }

    bool wait_while(const ::timespec* timeout_abs, std::function<bool()> wait_pred) {
	while (wait_pred()) {
	    if (!wait(timeout_abs))
		return false;
	}
	return true;
    }

    void lock_notify(std::function<void()> setup) {
	auto defer = lock();
	setup();
	notify();
    }

    void lock_notify_all(std::function<void()> setup) {
	auto defer = lock();
	setup();
	notify_all();
    }
};


} // namespace c7::thread


#endif // c7thread/condvar.hpp
