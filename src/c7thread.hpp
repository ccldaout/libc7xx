/*
 * c7thread.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1557099975
 */
#ifndef C7_THREAD_HPP_LOADED__
#define C7_THREAD_HPP_LOADED__
#include <c7common.hpp>


#include <c7delegate.hpp>
#include <c7utils.hpp>
#include <functional>
#include <system_error>
#include <string>


namespace c7 {
namespace thread {


struct thread_error: public std::system_error {
    thread_error(int err, const char *s):
	std::system_error(std::error_code(err, std::generic_category()), s) {}
};


/*----------------------------------------------------------------------------
                                   spinlock
----------------------------------------------------------------------------*/

class spinlock {
private:
    class impl;
    impl *pimpl;

public:
    spinlock(const spinlock&) = delete;
    spinlock& operator=(const spinlock&) = delete;

    spinlock();
    spinlock(spinlock&& o);
    spinlock& operator=(spinlock&& o);
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


/*----------------------------------------------------------------------------
                                    mutex
----------------------------------------------------------------------------*/

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
    mutex(mutex&& o);
    mutex& operator=(mutex&& o);
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


/*----------------------------------------------------------------------------
                              condition variable
----------------------------------------------------------------------------*/

class condvar {
private:
    class impl;
    impl *pimpl;

public:
    condvar(const condvar&) = delete;
    condvar& operator=(const condvar&) = delete;

    condvar();
    explicit condvar(mutex& mutex);
    condvar(condvar&&);
    condvar& operator=(condvar&&);
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


/*----------------------------------------------------------------------------
                                   thread
----------------------------------------------------------------------------*/

class proxy;

class thread {
private:
    class impl;
    std::shared_ptr<impl> pimpl;

    friend class proxy;
    friend struct self;

    void set_target(std::function<void()>&&);
    void set_finalize(std::function<void()>&&);

public:
    enum exit_type {
	NA_IDLE,	// thread is not started yet.
	NA_RUNNING,	// thread is running.
	EXIT,		// thread call self::exit() or return from target function.
	ABORT,		// thread call self::abort()
	CRASH,		// thread is terminated by exception.
    };

    c7::delegate<void, proxy>::proxy on_start;
    c7::delegate<void, proxy>::proxy on_finish;

    thread(thread&) = delete;
    thread& operator=(const thread&) = delete;

    thread();
    thread(thread&&);
    thread& operator=(thread&&);
    ~thread();
    void reuse();

    bool set_stacksize(size_t stack_b);
    void set_name(const std::string& name);

    template <typename F, typename... Args>
    void target(F func, Args... args) {
	set_target([=](){ func(args...); });
    }

    template <typename F, typename... Args>
    void finalize(F func, Args... args) {
	set_finalize([=](){ func(args...); });
    }

    c7::result<> start();

    bool join(c7::usec_t timeout = -1);
    exit_type status() const;
    c7::result<>& terminate_result();

    bool is_alive() const;
    bool is_self() const;
    const char *name() const;
    uint64_t id() const;
    void signal(int sig);
};

struct self {
    static const char *name();
    static uint64_t id();
    [[noreturn]] static void exit();
    [[noreturn]] static void abort();
    [[noreturn]] static void abort(c7::result<>&&);
};

class proxy {
private:
    thread::impl *pimpl;

public:
    explicit proxy(thread::impl *pimpl);
    explicit proxy(const thread& th);

    thread::exit_type status() const;
    const char *name() const;
    uint64_t id() const;
    void print(std::ostream& out, const std::string& format) const;

    bool operator==(const thread& t) const {
	return pimpl == t.pimpl.get();
    }
};

extern bool operator==(const thread& t, const proxy& p);


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace thread
} // namespace c7


#endif // c7thread.hpp
