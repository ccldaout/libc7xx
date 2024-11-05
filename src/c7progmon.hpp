/*
 * c7progmon.hpp
 *
 * Copyright (c) 2024 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sRGKUEcQJcS1wTQQZeiqmWhojdcRKboFxynjJCpwP3c/edit?usp=drive_link
 */
#ifndef C7_PROGMON_HPP_LOADED_
#define C7_PROGMON_HPP_LOADED_
#include <c7common.hpp>


#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <c7signal.hpp>
#include <c7utils.hpp>


namespace c7 {


template <typename Data>
class progress_monitor_base {
public:
    virtual ~progress_monitor_base() {}

    void update(const Data& d) {
	mm_addr_->updated = true;
	mm_addr_->data = d;
    }

    Data& update() {
	mm_addr_->updated = true;
	return mm_addr_->data;
    }

protected:
    struct shared_context {
	Data data;
	bool updated;
    };

    shared_context *mm_addr_;

    virtual void monitoring_init()  = 0;
    virtual void monitoring_check(uint64_t not_updated_count, pid_t parent_pid, Data&) = 0;

    void call_monitoring_check(uint64_t not_updated_count, pid_t parent_pid, shared_context *sp) {
	monitoring_check(not_updated_count, parent_pid, sp->data);
    }
};


template <>
class progress_monitor_base<void> {
public:
    virtual ~progress_monitor_base() {}

    void update() {
	mm_addr_->updated = true;
    }

protected:
    struct shared_context {
	bool updated;
    };

    shared_context *mm_addr_;

    virtual void monitoring_init()  = 0;
    virtual void monitoring_check(uint64_t not_updated_count, pid_t parent_pid) = 0;

    void call_monitoring_check(uint64_t not_updated_count, pid_t parent_pid, shared_context *) {
	monitoring_check(not_updated_count, parent_pid);
    }
};


template <typename Data = void>
class progress_monitor: public progress_monitor_base<Data> {
public:
    progress_monitor() {
	int prot  = PROT_READ|PROT_WRITE;
	int flags = MAP_SHARED|MAP_ANONYMOUS;
	void *p = ::mmap(nullptr, sizeof(shared_context), prot, flags, -1, 0);
	if (p == MAP_FAILED) {
	    auto s = c7::format("%{}", c7result_err(errno, "mmap failed"));
	    throw std::runtime_error(s);
	}
	mm_addr_ = reinterpret_cast<shared_context*>(p);
    }

    ~progress_monitor() {
	stop_monitor(0);
	::munmap(mm_addr_, sizeof(shared_context));
    }

    void start_monitor(c7::usec_t interval_ms) {
	ppid_ = getpid();
	if (pid_ == C7_SYSERR) {
	    pid_ = ::fork();
	    if (pid_ == 0) {
		monitoring(interval_ms);
	    }
	}
    }

    void stop_monitor(c7::usec_t duration_us = 1000) {
	if (pid_ != C7_SYSERR) {
	    auto pid = pid_;
	    pid_ = C7_SYSERR;
	    ::kill(pid, SIGTERM);
	    c7::sleep_us(duration_us);
	    ::kill(pid, SIGKILL);
	}
    }

protected:
    void monitoring_init() override {
	for (int i = 3; i < 256; i++) {
	    ::close(i);
	}
    }

private:
    using base_t = progress_monitor_base<Data>;
    using shared_context = typename base_t::shared_context;
    using base_t::mm_addr_;

    pid_t pid_  = C7_SYSERR;
    pid_t ppid_ = C7_SYSERR;
    uint64_t not_updated_ = 0;

    void monitoring(c7::usec_t interval_ms) {
	::sigset_t sigs;
	::sigemptyset(&sigs);
	c7::signal::set(sigs);

	monitoring_init();

	for (;;) {
	    c7::sleep_ms(interval_ms);
	    
	    if (mm_addr_->updated) {
		not_updated_ = 0;
	    } else {
		not_updated_++;
	    }
	    mm_addr_->updated = false;

	    if (not_updated_ != 0 && getppid() != ppid_) {
		ppid_ = C7_SYSERR;
	    }

	    base_t::call_monitoring_check(not_updated_, ppid_, mm_addr_);

	    if (ppid_ == C7_SYSERR) {
		std::quick_exit(EXIT_SUCCESS);
	    }
	}
    }
};


} // namespace c7


#endif // c7progmon.hpp
