/*
 * c7thread.cpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include "c7thread.hpp"
#include <atomic>
#include <exception>
#include <pthread.h>
#include <signal.h>	// pthread_kill
#include <stdexcept>


namespace c7 {
namespace thread {


/*----------------------------------------------------------------------------
                                    spinlock
----------------------------------------------------------------------------*/

class spinlock::impl {
private:
    pthread_spinlock_t m_;
    std::string name_;

public:
    impl(std::string name): name_(name) {
	(void)pthread_spin_init(&m_, PTHREAD_PROCESS_PRIVATE);
    }

    ~impl() {
	(void)pthread_spin_destroy(&m_);
    }

    bool _lock(bool wait) {
	int ret = wait ? pthread_spin_lock(&m_) : pthread_spin_trylock(&m_);
	if (ret != C7_SYSOK) {
	    if (ret == EBUSY)
		return false;
	    throw thread_error(ret, "pthread_spinlock_[try]lock");
	}
	return true;
    }

    void unlock() {
	(void)pthread_spin_unlock(&m_);
    }

    const std::string& name() {
	return name_;
    }

    pthread_spinlock_t& __pthread_spinlock() {
	return m_;
    }
};

spinlock::spinlock(std::string name)
{
    pimpl = new spinlock::impl(name);
}

spinlock::spinlock(spinlock&& o)
{
    pimpl = o.pimpl;
    o.pimpl = nullptr;
}

spinlock& spinlock::operator=(spinlock&& o)
{
    if (this != &o) {
	delete pimpl;
	pimpl = o.pimpl;
	o.pimpl = nullptr;
    }
    return *this;
}

spinlock::~spinlock()
{
    delete pimpl;
    pimpl = nullptr;
}

void spinlock::_lock()
{
    (void)pimpl->_lock(true);
}

bool spinlock::_trylock()
{
    return pimpl->_lock(false);
}

void spinlock::unlock()
{
    pimpl->unlock();
}

const std::string& spinlock::name()
{
    return pimpl->name();
}


/*----------------------------------------------------------------------------
                                    mutex
----------------------------------------------------------------------------*/

class mutex::impl {
private:
    pthread_mutex_t m_;
    std::string name_;

public:
    impl(std::string name, bool recursive): name_(name) {
	pthread_mutexattr_t attr;
	(void)pthread_mutexattr_init(&attr);
	if (recursive)
	    (void)pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	(void)pthread_mutex_init(&m_, &attr);
    }

    ~impl() {
	(void)pthread_mutex_destroy(&m_);
    }

    bool _lock(bool wait) {
	int ret = wait ? pthread_mutex_lock(&m_) : pthread_mutex_trylock(&m_);
	if (ret != C7_SYSOK) {
	    if (ret == EBUSY)
		return false;
	    throw thread_error(ret, "pthread_mutex_[try]lock");
	}
	return true;
    }

    void unlock() {
	(void)pthread_mutex_unlock(&m_);
    }

    const std::string& name() {
	return name_;
    }

    pthread_mutex_t& __pthread_mutex() {
	return m_;
    }
};

mutex::mutex(bool recursive)
{
    pimpl = new mutex::impl("", recursive);
}

mutex::mutex(std::string name, bool recursive)
{
    pimpl = new mutex::impl(name, recursive);
}

mutex::mutex(mutex&& o)
{
    pimpl = o.pimpl;
    o.pimpl = nullptr;
}

mutex& mutex::operator=(mutex&& o)
{
    if (this != &o) {
	delete pimpl;
	pimpl = o.pimpl;
	o.pimpl = nullptr;
    }
    return *this;
}

mutex::~mutex()
{
    delete pimpl;
    pimpl = nullptr;
}

void mutex::_lock()
{
    (void)pimpl->_lock(true);
}

bool mutex::_trylock()
{
    return pimpl->_lock(false);
}

void mutex::unlock()
{
    pimpl->unlock();
}

const std::string& mutex::name()
{
    return pimpl->name();
}


/*----------------------------------------------------------------------------
                              condition variable
----------------------------------------------------------------------------*/

class condvar::impl {
private:
    pthread_cond_t c_ = PTHREAD_COND_INITIALIZER;
    mutex::impl* mimpl_;
    bool allocated_;
    std::string name_;

public:
    impl(mutex* mutex, std::string name): name_(name) {
	allocated_ = (mutex == nullptr);
	if (mutex == nullptr) {
	    mimpl_ = new mutex::impl(name, false);
	} else
	    mimpl_ = mutex->pimpl;
    }

    ~impl() {
	(void)pthread_cond_destroy(&c_);
	if (allocated_)
	    delete mimpl_;
    }

    bool _lock(bool wait) {
	return mimpl_->_lock(wait);
    }

    void unlock() {
	mimpl_->unlock();
    }

    bool wait(const ::timespec *timeout_abs) {
	int ret;
	if (timeout_abs == nullptr) {
	    ret = pthread_cond_wait(&c_, &mimpl_->__pthread_mutex());
	} else {
	    do {
		ret = pthread_cond_timedwait(&c_, &mimpl_->__pthread_mutex(), timeout_abs);
	    } while (ret == EINTR);
	}
	if (ret != C7_SYSOK) {
	    if (ret == ETIMEDOUT)
		return false;
	    throw thread_error(ret, "pthread_cond_[timed]wait");
	}
	return true;
    }

    void notify() {
	int ret;
	if ((ret = pthread_cond_signal(&c_)) != C7_SYSOK)
	    throw thread_error(ret, "pthread_cond_signal");
    }

    void notify_all() {
	int ret;
	if ((ret = pthread_cond_broadcast(&c_)) != C7_SYSOK)
	    throw thread_error(ret, "pthread_cond_broadcast");
    }

    const std::string& name() {
	return name_;
    }
};

condvar::condvar(std::string name)
{
    pimpl = new condvar::impl(nullptr, name);
}

condvar::condvar(mutex& mutex, std::string name)
{
    pimpl = new condvar::impl(&mutex, name);
}

condvar::condvar(condvar&& o)
{
    pimpl = o.pimpl;
    o.pimpl = nullptr;
}

condvar& condvar::operator=(condvar&& o)
{
    if (this != &o) {
	delete pimpl;
	pimpl = o.pimpl;
	o.pimpl = nullptr;
    }
    return *this;
}

condvar::~condvar()
{
    delete pimpl;
    pimpl = nullptr;
}

void condvar::_lock()
{
    (void)pimpl->_lock(true);
}

bool condvar::_trylock()
{
    return pimpl->_lock(false);
}

void condvar::unlock()
{
    pimpl->unlock();
}

void condvar::wait()
{
    pimpl->wait(nullptr);
}

bool condvar::wait(const ::timespec* timeout_abs)
{
    return pimpl->wait(timeout_abs);
}

void condvar::notify()
{
    pimpl->notify();
}

void condvar::notify_all()
{
    pimpl->notify_all();
}

const std::string& condvar::name()
{
    return pimpl->name();
}


/*----------------------------------------------------------------------------
                                   thread
----------------------------------------------------------------------------*/

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

    c7::result<void> term_result_;
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
	} catch (...) {
	    exit_ = thread::CRASH;
	    term_result_ = c7result_err(EFAULT, "thread is terminated by exception.");
	}

	if (finalize_) {
	    finalize_();
	}
	on_finish_(proxy(this));

	cv_.lock_notify_all([this](){ state_ = FINISHED; });
    }

    static void* entry_point(void *__arg) {
	auto self = static_cast<impl*>(__arg);
	current_thread = self;
	self->thread();
	return nullptr;
    }

public:
    impl(const impl&) = delete;
    impl(impl&&) = delete;
    impl& operator=(const impl&) = delete;
    impl& operator=(impl&&) = delete;

    impl():
	id_(id_counter_++), name_("thread<" + std::to_string(id_) + ">"),
	exit_(thread::NA_IDLE), state_(IDLE), cv_(name_) {
	(void)pthread_attr_init(&attr_);
	(void)pthread_attr_setdetachstate(&attr_, PTHREAD_CREATE_DETACHED);
	if (this == &main_thread) {
	    current_thread = this;
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

    result<void> start() {
	auto defer = cv_.lock();

	if (state_ != IDLE) {
	    return c7result_err(EEXIST, "thread already running: %{}", proxy(this));
	}
	state_ = STARTING;

	int ret = pthread_create(&pthread_, &attr_, entry_point,
				 static_cast<void*>(this));
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

    c7::result<void>& terminate_result() {
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
	if (state_ == RUNNING)
	    ::pthread_kill(pthread_, sig);
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
    static void abort(c7::result<void>&& res) {
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
    pimpl(o.pimpl),
    on_start(pimpl->on_start_), on_finish(pimpl->on_finish_) {
    o.pimpl = nullptr;
}

thread& thread::operator=(thread&& o)
{
    if (this != &o) {
	if (pimpl->is_running()) {
	    throw std::runtime_error("assignment is not allowed: target thread is running.");
	}
	delete pimpl;
	pimpl = o.pimpl;
	o.pimpl = nullptr;
	on_start = std::move(o.on_start);
	on_finish = std::move(o.on_finish);
    }
    return *this;
}
    
thread::~thread()
{
    delete pimpl;
    pimpl = nullptr;
}

void thread::reuse()
{
    if (auto s = pimpl->status(); s != NA_IDLE) {
	if (s == NA_RUNNING) {
	    throw std::runtime_error("invalid reuse(): thread is running.");
	}
	*this = std::move(thread());
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

result<void> thread::start()
{
    return pimpl->start();
}

bool thread::join(c7::usec_t timeout)
{
    return pimpl->join(timeout);
}
    
thread::exit_type thread::status() const
{
    return pimpl->status();
}

c7::result<void>& thread::terminate_result()
{
    return pimpl->terminate_result();
}

bool thread::is_alive() const
{
    return pimpl->is_alive();
}

bool thread::is_self() const
{
    return pimpl == impl::current_thread;
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

void self::abort(c7::result<void>&& res)
{
    thread::impl::abort(std::move(res));
}


// proxy

proxy::proxy(thread::impl *pimpl): pimpl(pimpl) {}

proxy::proxy(const thread& th): pimpl(th.pimpl) {}

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

bool operator==(const thread& t, const proxy& p)
{
    return p == t;
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace thread


void format_traits<thread::proxy>::convert(
    std::ostream& out, const std::string& format, const thread::proxy& th)
{
    const char *exitv[] = {
	"N/A(idle)", "N/A(running)", "EXIT", "ABORT", "CRASH",
    };
    const char *exit_str = "?";
    if (0 <= th.status() && th.status() < c7_numberof(exitv))
	exit_str = exitv[th.status()];

    out << c7::format("thread<#%{},%{},%{}(%{})>",
		      th.id(), th.name(), exit_str, th.status());
}


} // namespace c7
