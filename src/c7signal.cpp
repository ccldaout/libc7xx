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
	for (auto& h: handlers_) {
	    h = sig_default;
	}
	(void)::sigemptyset(&user_mask_);
	(void)::sigemptyset(&blocked_mask_);

	return start_sigwait_loop();
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

    void (*handler(int sig, void (*new_handler)(int)))(int) {
	auto old = handlers_[sig];
	if (new_handler == SIG_IGN) {
	    (void)::signal(sig, new_handler);
	} else {
	    if (new_handler == SIG_DFL) {
		new_handler = sig_default;
	    }
	}
	handlers_[sig] = new_handler;
	if (old == sig_default) {
	    old = SIG_DFL;
	}
	return old;
    }

    ::sigset_t block(const ::sigset_t& mask) {
	auto unlock = lock_.lock();
	auto old_mask = user_mask_;
	sigset_on(user_mask_, mask);
	return old_mask;
    }

    ::sigset_t unblock(const ::sigset_t& mask) {
	auto unlock = lock_.lock();
	auto old_mask = user_mask_;
	sigset_off(user_mask_, mask);
	check_blocked();
	return old_mask;
    }

    ::sigset_t setmask(const ::sigset_t& mask) {
	auto unlock = lock_.lock();
	auto old_mask = user_mask_;
	user_mask_ = mask;
	check_blocked();
	return old_mask;
    }

private:
    ::sigset_t blocked_mask_;
    ::sigset_t user_mask_;
    void (*handlers_[NSIG])(int);
    c7::thread::mutex lock_;

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

    void check_blocked() {
	for (int i = 0; i < NSIG; i++) {
	    if (::sigismember(&blocked_mask_, i) && !::sigismember(&user_mask_, i)) {
		(void)::kill(getpid(), i);
		::sigdelset(&blocked_mask_, i);
	    }
	}
    }

    void loop(::sigset_t waitmask) {
	for (;;) {
	    int sig;
	    if (::sigwait(&waitmask, &sig) == C7_SYSOK) {
		auto unlock = lock_.lock();
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
    signal_manager.start_sigwait_loop();
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
