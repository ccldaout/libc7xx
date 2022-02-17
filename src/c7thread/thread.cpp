/*
 * c7thread/thread.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <atomic>
#include <signal.h>	// pthread_kill
#include "condvar.hpp"
#include "thread.hpp"
#include "_private.hpp"


namespace c7::thread {


bool process_alive = true;


static void mark_exiting()
{
    process_alive = false;
}


class thread::impl {
    // ------------ types ------------
private:
    class exit_thread: public std::exception {};
    class abort_thread: public std::exception {};
    class duplicate_start: public std::exception {};

    enum state_t {
	IDLE,		// thread object is created but thread::impl::start is not called.
	STARTING,	// thread::impl::start is called but thread() is not started.
	START_FAILED,	// failed to run (per-thread initializer failed)
	RUNNING,	// target() is running.
	FINISHED,	// both target() and finalize() was finished (pthread was finished).
    };

    // ------------ static data ------------
public:
    static impl main_thread;
    static thread_local impl* current_thread;
    static std::atomic<uint64_t> id_counter_;

    // ------------ instance data ------------
private:
    uint64_t id_;
    std::string name_;
    pthread_attr_t attr_;
    pthread_t pthread_;

    c7::result<> term_result_;
    exit_type exit_;
    state_t state_;
    condvar cv_;

    std::function<void()> target_;
    std::function<void()> finalize_;

public:
    c7::delegate<void, proxy> on_start_;
    c7::delegate<void, proxy> on_finish_;

    // ------------ functions ------------
private:
    void thread() {
	cv_.lock_notify_all([this](){ state_ = RUNNING; });

	try {
	    exit_ = thread::NA_RUNNING;
	    on_start_(proxy(this));
	    target_();
	    exit_ = thread::EXIT;
	} catch (exit_thread&) {
	    exit_ = thread::EXIT;
	} catch (abort_thread&) {
	    exit_ = thread::ABORT;
	} catch (std::exception& e) {
	    exit_ = thread::CRASH;
	    term_result_ = c7result_err(EFAULT, "thread is terminated by exception: %{}", e.what());
	} catch (...) {
	    exit_ = thread::CRASH;
	    term_result_ = c7result_err(EFAULT, "thread is terminated by unknown exception.");
	}

	if (finalize_) {
	    finalize_();
	}
	on_finish_(proxy(this));

	cv_.lock_notify_all([this](){ state_ = FINISHED; });
    }

    static void* entry_point(void *__arg) {
	auto spp = static_cast<std::shared_ptr<impl>*>(__arg);
	auto shared_this = *spp;
	current_thread = shared_this.get();
	shared_this->thread();
	return nullptr;
    }

public:
    impl(const impl&) = delete;
    impl(impl&&) = delete;
    impl& operator=(const impl&) = delete;
    impl& operator=(impl&&) = delete;

    impl():
	id_(id_counter_++), name_("th<" + std::to_string(id_) + ">"),
	exit_(thread::NA_IDLE), state_(IDLE) {
	(void)pthread_attr_init(&attr_);
	(void)pthread_attr_setdetachstate(&attr_, PTHREAD_CREATE_DETACHED);
	if (this == &main_thread) {
	    current_thread = this;
	    name_ = "main";
	}
	if (id_ == 0) {
	    std::atexit(mark_exiting);
	}
    }

    ~impl() {
	(void)pthread_attr_destroy(&attr_);
    }

    bool set_stacksize(size_t stack_b) {
	return (pthread_attr_setstacksize(&attr_, stack_b) == C7_SYSOK);
    }

    void set_name(const std::string& name) {
	name_ = name;
    }

    void set_target(std::function<void()>&& target) {
	target_ = std::move(target);
    }

    void set_finalize(std::function<void()>&& finalize) {
	finalize_ = std::move(finalize);
    }

    result<> start(std::shared_ptr<impl> *self) {
	auto defer = cv_.lock();

	if (state_ != IDLE) {
	    return c7result_err(EEXIST, "thread already running: %{}", proxy(this));
	}
	state_ = STARTING;

	int ret = pthread_create(&pthread_, &attr_, entry_point,
				 static_cast<void*>(self));
	if (ret != C7_SYSOK) {
	    state_ = START_FAILED;
	    if (finalize_) {
		finalize_();
	    }
	    return c7result_err(ret, "cannot create thread: %{}", proxy(this));
	}

	cv_.wait_while([this](){ return state_ == STARTING; });
	return c7result_ok();
    }

    bool join(c7::usec_t timeout) {
	::timespec *tmsp = c7::mktimespec(timeout);
	auto defer = cv_.lock();
	if (state_ == IDLE) {
	    return false;
	}
	return cv_.wait_while(tmsp, [this](){ return state_ != FINISHED; });
    }

    thread::exit_type status() {
	return exit_;
    }

    c7::result<>& terminate_result() {
	return term_result_;
    }

    bool is_alive() {
	return (state_ != IDLE && state_ != FINISHED);
    }

    bool is_running() {
	return (state_ == STARTING || state_ == RUNNING);
    }

    const char *name() {
	return name_.c_str();
    }

    uint64_t id() {
	return id_;
    }

    void signal(int sig) {
	if (state_ == RUNNING) {
	    ::pthread_kill(pthread_, sig);
	}
    }

    [[noreturn]]
    static void exit() {
	throw exit_thread();
    }

    [[noreturn]]
    static void abort() {
	throw abort_thread();
    }

    [[noreturn]]
    static void abort(c7::result<>&& res) {
	current_thread->term_result_ = std::move(res);
	throw abort_thread();
    }
};


std::atomic<uint64_t> thread::impl::id_counter_;
thread::impl  thread::impl::main_thread;
thread_local thread::impl* thread::impl::current_thread;


// thread class

thread::thread():
    pimpl(new thread::impl()),
    on_start(pimpl->on_start_), on_finish(pimpl->on_finish_) {
}

thread::thread(thread&& o):
    pimpl(std::move(o.pimpl)),
    on_start(pimpl->on_start_), on_finish(pimpl->on_finish_) {
}

thread& thread::operator=(thread&& o)
{
    if (this != &o) {
	pimpl = std::move(o.pimpl);
	on_start = std::move(o.on_start);
	on_finish = std::move(o.on_finish);
    }
    return *this;
}
    
thread::~thread() {}

void thread::reuse()
{
    if (auto s = pimpl->status(); s != NA_IDLE) {
	if (s == NA_RUNNING) {
	    throw std::runtime_error("invalid reuse(): thread is running.");
	}
	*this = thread();
    }
}

bool thread::set_stacksize(size_t stack_b)
{
    return pimpl->set_stacksize(stack_b);
}

void thread::set_name(const std::string& name)
{
    pimpl->set_name(name);
}

void thread::set_target(std::function<void()>&& target)
{
    pimpl->set_target(std::move(target));
}

void thread::set_finalize(std::function<void()>&& finalize)
{
    pimpl->set_finalize(std::move(finalize));
}

result<> thread::start()
{
    return pimpl->start(&pimpl);
}

bool thread::join(c7::usec_t timeout)
{
    return pimpl->join(timeout);
}
    
thread::exit_type thread::status() const
{
    return pimpl->status();
}

c7::result<>& thread::terminate_result()
{
    return pimpl->terminate_result();
}

bool thread::is_alive() const
{
    return pimpl->is_alive();
}

bool thread::is_self() const
{
    return pimpl.get() == impl::current_thread;
}

const char *thread::name() const
{
    return pimpl->name();
}

uint64_t thread::id() const
{
    return pimpl->id();
}

void thread::signal(int sig)
{
    pimpl->signal(sig);
}


// self

const char *self::name()
{
    return thread::impl::current_thread->name();
}

uint64_t self::id()
{
    return thread::impl::current_thread->id();
}

void self::exit()
{
    thread::impl::exit();
}

void self::abort()
{
    thread::impl::abort();
}

void self::abort(c7::result<>&& res)
{
    thread::impl::abort(std::move(res));
}


// proxy

proxy::proxy(thread::impl *pimpl): pimpl(pimpl) {}

proxy::proxy(const thread& th): pimpl(th.pimpl.get()) {}

thread::exit_type proxy::status() const
{
    return pimpl->status();
}

const char *proxy::name() const
{
    return pimpl->name();
}

uint64_t proxy::id() const
{
    return pimpl->id();
}

void proxy::print(std::ostream& out, const std::string&) const
{
    const char *exitv[] = {
	"N/A(idle)", "N/A(running)", "EXIT", "ABORT", "CRASH",
    };
    const char *exit_str = "?";
    auto sts = status();
    if (0 <= sts && sts < c7_numberof(exitv)) {
	exit_str = exitv[sts];
    }
    c7::format(out, "thread<#%{},%{},%{}(%{})>",
	       id(), name(), exit_str, sts);
}

bool operator==(const thread& t, const proxy& p)
{
    return p == t;
}


} // namespace c7::thread
