/*
 * c7thread/thread.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1557099975
 */
#ifndef C7_THREAD_THREAD_HPP_LOADED__
#define C7_THREAD_THREAD_HPP_LOADED__
#include <c7common.hpp>


#include <c7delegate.hpp>
#include <c7utils.hpp>
#include <functional>
#include <system_error>
#include <string>


namespace c7::thread {


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
    static thread::exit_type status();
    static const c7::result<>& terminate_result();
    static std::string to_string();

    [[noreturn]] static void exit();
    [[noreturn]] static void abort();
    [[noreturn]] static void abort(c7::result<>&&);
};


class proxy {
private:
    std::shared_ptr<thread::impl> pimpl;

public:
    explicit proxy(std::shared_ptr<thread::impl> pimpl);
    explicit proxy(const thread& th);

    const char *name() const;
    uint64_t id() const;
    thread::exit_type status() const;
    const c7::result<>& terminate_result() const;

    void print(std::ostream& out, const std::string& format) const;

    bool operator==(const thread& t) const {
	return pimpl == t.pimpl;
    }
};

extern bool operator==(const thread& t, const proxy& p);


} // namespace c7::thread


#endif // c7thread/thread.hpp
