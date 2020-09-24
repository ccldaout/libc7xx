/*
 * c7signal.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7signal.hpp>
#include <c7threadsync.hpp>
#include <atomic>
#include <cstring>


namespace c7 {
namespace signal {


#define _CNTL_SIG	SIGCHLD	// signal for restarting sgiwait


class sigmanager {
private:
    struct action_t {
	void (*handler)(int sig);
	::sigset_t sigmask;
    };

    action_t action_[NSIG];
    c7::thread::mutex lock_;
    c7::thread::event applied_;
    c7::thread::thread thread_;
    ::sigset_t block_sigs_;	// signals to be blocked just before sigwait
    ::sigset_t wait_sigs_;	// signals to be passed to sigwait
    ::sigset_t managed_sigs_;	// all managed signals

    static void handle_SIGCHLD(int sig) {}

    void monitor() {
	for (;;) {
	    ::sigset_t blocksigs, waitsigs;
	    {
		auto defer = lock_.lock();
		blocksigs = block_sigs_;
		waitsigs  = wait_sigs_;
	    }
	    (void)::pthread_sigmask(SIG_SETMASK, &blocksigs, nullptr);
	    applied_.set();

	    int sig1;
	    if (::sigwait(&waitsigs, &sig1) == C7_SYSOK) {
		action_t cb;
		{
		    auto defer = lock_.lock();
		    cb = action_[sig1 - 1];
		}
		if (cb.handler != nullptr) {
		    (void)::pthread_sigmask(SIG_BLOCK, &cb.sigmask, nullptr);
		    cb.handler(sig1);
		}
	    }
	}
    }

    void apply_settings() {
	// Send dummy signal to monitor thread to reload new signal masks
	applied_.clear();
	thread_.signal(_CNTL_SIG);
	applied_.wait();
    }

    void start_monitor() {
	// setup SIGCHLD handler.
	// [CAUTION] DON'T USE sigmanager's member function.
	auto& cb = action_[SIGCHLD - 1];
	cb.handler = handle_SIGCHLD;
	(void)::sigemptyset(&cb.sigmask);
	(void)::sigaddset(&cb.sigmask,    SIGCHLD);
	(void)::sigaddset(&block_sigs_,   SIGCHLD);
	(void)::sigaddset(&wait_sigs_,    SIGCHLD);
	(void)::sigaddset(&managed_sigs_, SIGCHLD);

	sigset_t main_sigs;
	(void)::sigfillset(&main_sigs);
	(void)::sigdelset(&main_sigs, SIGSEGV);
	(void)::sigdelset(&main_sigs, SIGBUS);
	(void)::sigdelset(&main_sigs, SIGABRT);
	(void)::sigprocmask(SIG_SETMASK, &main_sigs, nullptr);

	thread_.set_name("sigmanager");
	thread_.target([this](){ monitor();});
	if (!thread_.start()) {
	    p_("pthread_create failure");
	    abort();
	}
    }

    inline void start_if() {
	static std::atomic<bool> started(false);
	if (!started.exchange(true)) {
	    start_monitor();
	}
    }

    friend void __enable(void (*)(int));

public:
    sigmanager() {
	(void)std::memset(action_, 0, sizeof(action_));
	(void)::sigemptyset(&block_sigs_);
	(void)::sigemptyset(&wait_sigs_);
	(void)::sigemptyset(&managed_sigs_);

	(void)::sigaddset(&block_sigs_, _CNTL_SIG);
	(void)::sigaddset(&wait_sigs_,  _CNTL_SIG);
    }

    c7::defer lock() {
	return lock_.lock();
    }

    result<void(*)(int)> handle(int sig,
				void (*handler)(int),
				const ::sigset_t& sigmask_on_call) {
	start_if();

	if (sig < 1 || NSIG < sig) {
	    return c7result_err(EINVAL, "handle: sig:%{} is out of range", sig);
	}
	
	void (*o_handler)(int);

	{
	    auto defer = lock_.lock();
	    auto& cb = action_[sig - 1];
	    o_handler = cb.handler;

	    cb.handler = handler;
	    cb.sigmask = sigmask_on_call;

	    if (handler == SIG_IGN || handler == SIG_DFL) {
		(void)::signal(sig, handler);
		(void)::sigdelset(&block_sigs_,   sig);
		(void)::sigdelset(&wait_sigs_,    sig);
		(void)::sigdelset(&managed_sigs_, sig);
	    } else {
		(void)::sigaddset(&block_sigs_,   sig);
		(void)::sigaddset(&wait_sigs_,    sig);
		(void)::sigaddset(&managed_sigs_, sig);
	    }
	}

	apply_settings();
	return c7result_ok(o_handler);
    }

    ::sigset_t update_mask(int how, const ::sigset_t& sigs) {
	start_if();

	::sigset_t o_sigs;

	{
	    auto defer = lock_.lock();
	    o_sigs = block_sigs_;

	    if (how == SIG_BLOCK) {
		for (int sig0 = 0; sig0 < NSIG; sig0++) {
		    int sig1 = sig0 + 1;
		    if (sig1 == _CNTL_SIG)
			continue;
		    if (::sigismember(&sigs, sig1)) {
			::sigaddset(&block_sigs_, sig1);
			::sigdelset(&wait_sigs_, sig1);
		    }
		}
	    } else if (how == SIG_UNBLOCK) {
		for (int sig0 = 0; sig0 < NSIG; sig0++) {
		    int sig1 = sig0 + 1;
		    if (sig1 == _CNTL_SIG)
			continue;
		    if (::sigismember(&sigs, sig1)) {
			if (::sigismember(&managed_sigs_, sig1)) {
			    ::sigaddset(&block_sigs_, sig1);
			    ::sigaddset(&wait_sigs_, sig1);
			} else {
			    ::sigdelset(&block_sigs_, sig1);
			    //::sigdelset(&wait_sigs_, sig1);
			}
		    }
		}
	    } else if (how == SIG_SETMASK) {
		for (int sig0 = 0; sig0 < NSIG; sig0++) {
		    int sig1 = sig0 + 1;
		    if (sig1 == _CNTL_SIG)
			continue;
		    if (::sigismember(&sigs, sig1)) {
			::sigaddset(&block_sigs_, sig1);
			if (::sigismember(&managed_sigs_, sig1)) {
			    ::sigaddset(&wait_sigs_, sig1);
			} else {
			    //::sigdelset(&wait_sigs_, sig1);
			}
		    } else {
			if (::sigismember(&managed_sigs_, sig1)) {
			    ::sigaddset(&block_sigs_, sig1);
			    ::sigaddset(&wait_sigs_, sig1);
			} else {
			    ::sigdelset(&block_sigs_, sig1);
			    //::sigaddset(&wait_sigs_, sig1);
			}
		    }
		}
	    }
	}

	apply_settings();
	return o_sigs;
    }
};

static sigmanager manager;


void __enable(void (*sigchld)(int))
{
    manager.start_if();
    manager.action_[SIGCHLD - 1].handler = sigchld;
}


c7::defer lock()
{
    return manager.lock();
}


c7::result<void(*)(int)>
handle(int sig, void (*handler)(int), const ::sigset_t& sigmask_on_call)
{
    return manager.handle(sig, handler, sigmask_on_call);
}


::sigset_t mask(int how, const ::sigset_t& sigs)
{
    return manager.update_mask(how, sigs);
}


c7::defer block(void)
{
    const int sigs[] = {
	SIGINT, SIGTERM, SIGHUP, SIGABRT, SIGQUIT, SIGTSTP, SIGUSR1, SIGUSR2, SIGWINCH
    };
    ::sigset_t n_set;
    (void)::sigemptyset(&n_set);
    for (auto sig: sigs)
	(void)::sigaddset(&n_set, sig);
    auto o_sigs = manager.update_mask(SIG_BLOCK, n_set);
    return c7::defer(unblock, o_sigs);
}


} // namespace signal
} // namespace c7
