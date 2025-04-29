/*
 * c7signal.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7app.hpp>
#include <c7signal.hpp>
#include <c7thread.hpp>
#include <unistd.h>
#include <sys/signal.h>
#include <mutex>


namespace c7 {
namespace signal {


class sigmanager {
public:
    sigmanager(const sigmanager&) = delete;
    sigmanager& operator=(const sigmanager&) = delete;
    sigmanager(sigmanager&&) = delete;
    sigmanager& operator=(sigmanager&&) = delete;

    sigmanager() = default;

    ~sigmanager() = default;

    result<> init() {
	init_mutex();

	for (auto& h: handlers_) {
	    h = sig_default;
	}
	(void)::sigemptyset(&user_mask_);
	(void)::sigemptyset(&blocked_mask_);

	return start_sigwait_loop();
    }

    void atfork() {
	// [IMPORTANT] prevent some thread on forked process from blocking
	//             at unlock mutex pointed by lock_.
	init_mutex();
	start_sigwait_loop();
    }

    void (*handler(int sig, void (*new_h)(int)))(int) {
	if (new_h == SIG_DFL || new_h == SIG_IGN) {
	    ::signal(sig, new_h);
	} else {
	    ::signal(sig, sig_noop);
	}
	auto old_h = handlers_[sig];
	handlers_[sig] = (new_h == SIG_DFL) ? sig_default : new_h;

	return (old_h == sig_default) ? SIG_DFL : old_h;
    }

    sig_handler handler(int sig, sig_handler new_h) {
	auto old_h = handler(sig, sig_extention);
	auto old_ext = sig_handlers_[sig];
	sig_handlers_[sig] = new_h;
	if (old_h == sig_extention) {
	    return old_ext;
	} else {
	    return sig_handler{old_h};
	}
    }

    ::sigset_t block(const ::sigset_t& mask) {
	auto unlock = lock_->lock();
	auto old_mask = user_mask_;
	sigset_on(user_mask_, mask);
	return old_mask;
    }

    ::sigset_t unblock(const ::sigset_t& mask) {
	auto unlock = lock_->lock();
	auto old_mask = user_mask_;
	sigset_off(user_mask_, mask);
	check_blocked();
	return old_mask;
    }

    ::sigset_t setmask(const ::sigset_t& mask) {
	auto unlock = lock_->lock();
	auto old_mask = user_mask_;
	user_mask_ = mask;
	check_blocked();
	return old_mask;
    }

private:
    ::sigset_t blocked_mask_;
    ::sigset_t user_mask_;
    void (*handlers_[NSIG])(int);
    static sig_handler sig_handlers_[NSIG];
    std::unique_ptr<c7::thread::mutex> lock_;


    static void sigset_on(::sigset_t& mask, const ::sigset_t& on) {
	for (int i = 0; i < NSIG; i++) {
	    if (::sigismember(&on, i)) {
		::sigaddset(&mask, i);
	    }
	}
    }

    static void sigset_off(::sigset_t& mask, const ::sigset_t& off) {
	for (int i = 0; i < NSIG; i++) {
	    if (::sigismember(&off, i)) {
		::sigdelset(&mask, i);
	    }
	}
    }

    static void sigset_em(::sigset_t& mask) {
	int sigs[] = { SIGSEGV, SIGBUS, SIGILL, SIGBUS, SIGABRT };
	for (auto sig: sigs) {
	    sigdelset(&mask, sig);
	}
    }

    static void sig_default(int sig) {
	::sigset_t mask;
	(void)::sigemptyset(&mask);
	(void)::sigaddset(&mask, sig);
	(void)pthread_sigmask(SIG_UNBLOCK, &mask, nullptr);
	(void)::kill(::getpid(), sig);
    }

    static void sig_extention(int sig) {
	sig_handlers_[sig](sig);
    }

    static void sig_noop(int) {}


    void init_mutex() {
	lock_ = std::make_unique<c7::thread::mutex>();
    }

    void check_blocked() {
	for (int i = 0; i < NSIG; i++) {
	    if (::sigismember(&blocked_mask_, i) && !::sigismember(&user_mask_, i)) {
		(void)::kill(getpid(), i);
		::sigdelset(&blocked_mask_, i);
	    }
	}
    }

    result<> start_sigwait_loop() {

	::sigset_t wait_mask;
	(void)::sigfillset(&wait_mask);
	sigset_em(wait_mask);
	(void)::sigprocmask(SIG_SETMASK, &wait_mask, nullptr);

	c7::thread::thread loop_thread;
	loop_thread.target([this, wait_mask](){ loop(wait_mask); });
	loop_thread.set_name("sigloop");
	return loop_thread.start();
    }

    void loop(::sigset_t waitmask) {
	for (;;) {
	    int sig;
	    if (::sigwait(&waitmask, &sig) == C7_SYSOK) {
		auto unlock = lock_->lock();
		if (::sigismember(&user_mask_, sig)) {
		    ::sigaddset(&blocked_mask_, sig);
		} else {
		    unlock();
		    handlers_[sig](sig);
		}
	    } else {
		c7error(c7result_err(errno, "sigwait() failed"));
	    }
	}
    }
};


sig_handler sigmanager::sig_handlers_[NSIG];
static sigmanager signal_manager;
static volatile int once_flag;

static void init()
{
    if (auto res = signal_manager.init(); !res) {
	c7abort(res, "signal_manager initialization failed.");
    }
}

static void atfork()
{
    signal_manager.atfork();
}

static void call_init_once()
{
    if (__sync_lock_test_and_set(&once_flag, 1) == 0) {
	init();
	pthread_atfork(nullptr, nullptr, atfork);
    }
}

void enable_SIGCHLD(void (*sigchld)(int))
{
    call_init_once();
    signal_manager.handler(SIGCHLD, sigchld);
}


c7::result<void(*)(int)>
handle(int sig, void (*handler)(int), const ::sigset_t&)
{
    call_init_once();
    return c7result_ok(signal_manager.handler(sig, handler));
}


c7::result<sig_handler>
handle(int sig, sig_handler handler, const ::sigset_t&)
{
    call_init_once();
    return c7result_ok(signal_manager.handler(sig, handler));
}


::sigset_t set(const ::sigset_t& sigs)
{
    call_init_once();
    return signal_manager.setmask(sigs);
}


::sigset_t unblock(const ::sigset_t& sigs)
{
    call_init_once();
    return signal_manager.unblock(sigs);
}


::sigset_t block(const ::sigset_t& sigs)
{
    call_init_once();
    return signal_manager.block(sigs);
}


::sigset_t mask(int how, const ::sigset_t& sigs)
{
    switch (how) {
    case SIG_SETMASK:
	return set(sigs);
    case SIG_UNBLOCK:
	return unblock(sigs);
    default:
	return block(sigs);
    }
}


c7::defer block(void)
{
    const int sigs[] = {
	SIGINT, SIGTERM, SIGHUP, SIGABRT, SIGQUIT, SIGTSTP, SIGUSR1, SIGUSR2, SIGWINCH
    };
    ::sigset_t n_set;
    (void)::sigemptyset(&n_set);
    for (auto sig: sigs) {
	(void)::sigaddset(&n_set, sig);
    }
    auto o_set = block(n_set);
    return c7::defer(set, o_set);
}


} // namespace signal
} // namespace c7
