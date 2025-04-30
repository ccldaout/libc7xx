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
#include <c7utils/time.hpp>
#include "condvar.hpp"
#include "thread.hpp"
#include "_private.hpp"


namespace c7::thread {


bool process_alive = true;


static void mark_exiting()
{
    process_alive = false;
}


/*----------------------------------------------------------------------------
                                 thread::impl
----------------------------------------------------------------------------*/

class thread::impl: public std::enable_shared_from_this<thread::impl> {
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
	FINISHED,	// both target() and finalize() finished (pthread finished).
    };

    // ------------ static data ------------
    static std::atomic<uint64_t> id_counter_;
    static impl main_thread_;
    static impl undef_thread_;
    static thread_local impl* current_thread_;

    // ------------ instance data ------------
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
    void thread();
    static void* entry_point(void *arg);

public:
    impl(const impl&) = delete;
    impl(impl&&) = delete;
    impl& operator=(const impl&) = delete;
    impl& operator=(impl&&) = delete;

    impl();

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

    result<> start();

    bool join(c7::usec_t timeout);

    thread::exit_type status() const {
	return exit_;
    }

    c7::result<>& terminate_result() {
	return term_result_;
    }

    const c7::result<>& terminate_result() const {
	return term_result_;
    }

    bool is_alive() const {
	auto s = state_;
	return (s != IDLE && s != FINISHED);
    }

    bool is_running() const {
	auto s = state_;
	return (s == STARTING || s == RUNNING);
    }

    const char *name() const {
	return name_.c_str();
    }

    uint64_t id() const {
	return id_;
    }

    void signal(int sig) const {
	if (state_ == RUNNING) {
	    ::pthread_kill(pthread_, sig);
	}
    }

    void print(std::ostream& out, const std::string&) const;

    static void link_current(impl *pimpl) {
	pimpl->id_ = id_counter_++;
	if (pimpl->name_.empty()) {
	    pimpl->name_ = "th<" + std::to_string(pimpl->id_) + ">";
	}
	current_thread_ = pimpl;
    }

    static impl *current() {
	return current_thread_;
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
	current_thread_->term_result_ = std::move(res);
	throw abort_thread();
    }
};


std::atomic<uint64_t> thread::impl::id_counter_ = 2;
thread::impl thread::impl::main_thread_;
thread::impl thread::impl::undef_thread_;
thread_local thread::impl* thread::impl::current_thread_ = &thread::impl::undef_thread_;


thread::impl::impl():
    id_(0),
    exit_(thread::NA_IDLE),
    state_(IDLE)
{
    (void)pthread_attr_init(&attr_);
    (void)pthread_attr_setdetachstate(&attr_, PTHREAD_CREATE_DETACHED);
    if (this == &main_thread_) {
	id_   = 1;
	name_ = "main";
	current_thread_ = this;
	std::atexit(mark_exiting);
    } else if (this == &undef_thread_) {
	id_   = 0;
	name_ = "?";
    } else if (current_thread_ == &undef_thread_) {
	link_current(this);
    }
}


result<>
thread::impl::start()
{
    auto defer = cv_.lock();

    if (state_ != IDLE) {
	return c7result_err(EEXIST, "thread already running: %{}", *this);
    }
    state_ = STARTING;

    auto self_sp = shared_from_this();
    int ret = pthread_create(&pthread_, &attr_, entry_point, &self_sp);
    if (ret != C7_SYSOK) {
	state_ = START_FAILED;
	if (finalize_) {
	    finalize_();
	}
	return c7result_err(ret, "cannot create thread: %{}", *this);
    }

    cv_.wait_while([this](){ return state_ == STARTING; });
    return c7result_ok();
}


void*
thread::impl::entry_point(void *arg)
{
    auto spp = static_cast<std::shared_ptr<impl>*>(arg);
    auto shared_this = *spp;
    link_current(shared_this.get());
    shared_this->thread();
    return nullptr;
}


void
thread::impl::thread()
{
    cv_.lock_notify_all([this](){ state_ = RUNNING; });

    try {
	exit_ = thread::NA_RUNNING;
	term_result_ = c7result_err(EINPROGRESS, "thread (%{}) is not finished.", *this);

	on_start_(proxy(shared_from_this()));
	target_();

	exit_ = thread::EXIT;
	term_result_.clear();
    } catch (exit_thread&) {
	exit_ = thread::EXIT;
	term_result_.clear();
    } catch (abort_thread&) {
	exit_ = thread::ABORT;
	// term_result_ is set on thread::impl::abort()
    } catch (std::exception& e) {
	exit_ = thread::CRASH;
	term_result_ = c7result_err(EFAULT, "thread (%{}) is terminated by exception:\n%{}",
				    *this, e.what());
    } catch (...) {
	exit_ = thread::CRASH;
	term_result_ = c7result_err(EFAULT, "thread (%{}) is terminated by unknown exception.",
				    *this);
    }

    try {
	if (finalize_) {
	    finalize_();
	}
	on_finish_(proxy(shared_from_this()));
    } catch (...) {
	exit_ = thread::CRASH;
	term_result_ << c7result_err(EFAULT, "thread (%{}) is terminated by exception on finalize.",
				    *this);
    }

    cv_.lock_notify_all([this](){ state_ = FINISHED; });
}


bool
thread::impl::join(c7::usec_t timeout)
{
    ::timespec *tmsp = c7::mktimespec(timeout);
    auto defer = cv_.lock();
    if (state_ == IDLE) {
	return false;
    }
    return cv_.wait_while(tmsp, [this](){ return state_ != FINISHED; });
}


void
thread::impl::print(std::ostream& out, const std::string&) const
{
    static const char * const exitv[] = {
	"N/A(idle)", "N/A(running)", "EXIT", "ABORT", "CRASH",
    };
    const char *exit_str = "?";
    if (0 <= exit_ && exit_ < c7_numberof(exitv)) {
	exit_str = exitv[exit_];
    }
    c7::format(out, "thread<#%{},%{},%{}(%{})>", id_, name_, exit_str, exit_);
}


/*----------------------------------------------------------------------------
                                    thread
----------------------------------------------------------------------------*/

thread::thread():
    pimpl(new thread::impl()),
    on_start(pimpl->on_start_), on_finish(pimpl->on_finish_)
{
}

thread::thread(thread&& o):
    pimpl(std::move(o.pimpl)),
    on_start(pimpl->on_start_), on_finish(pimpl->on_finish_)
{
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
    return pimpl.get() == impl::current();
}

const char *thread::name() const
{
    return pimpl->name();
}

uint64_t thread::id() const
{
    return pimpl->id();
}

void thread::signal(int sig) const
{
    pimpl->signal(sig);
}


/*----------------------------------------------------------------------------
                                     self
----------------------------------------------------------------------------*/

const char *self::name()
{
    return thread::impl::current()->name();
}

uint64_t self::id()
{
    return thread::impl::current()->id();
}

thread::exit_type self::status()
{
    return thread::impl::current()->status();
}

const c7::result<>& self::terminate_result()
{
    return thread::impl::current()->terminate_result();
}

std::string self::to_string()
{
    std::stringstream ss;
    thread::impl::current()->print(ss, "");
    return ss.str();
}

void self::exit()
{
    thread::impl::exit();
}

void self::abort()
{
    self::abort(c7result_err(ECANCELED,
			     "thread (%{}) is aborted by self.",
			     *thread::impl::current()));
}

void self::abort(c7::result<>&& res)
{
    thread::impl::abort(std::move(res));
}


/*----------------------------------------------------------------------------
                                    proxy
----------------------------------------------------------------------------*/

proxy::proxy(std::shared_ptr<thread::impl> pimpl): pimpl(pimpl) {}

proxy::proxy(const thread& th): pimpl(th.pimpl) {}

const char *proxy::name() const
{
    return pimpl->name();
}

uint64_t proxy::id() const
{
    return pimpl->id();
}

thread::exit_type proxy::status() const
{
    return pimpl->status();
}

const c7::result<>& proxy::terminate_result() const
{
    return pimpl->terminate_result();
}

void proxy::print(std::ostream& out, const std::string& fmt) const
{
    pimpl->print(out, fmt);
}

bool operator==(const thread& t, const proxy& p)
{
    return p == t;
}


} // namespace c7::thread
