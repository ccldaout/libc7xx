/*
 * c7event/timer.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7event/timer.hpp>
#include <unistd.h>
#include <sys/timerfd.h>


namespace c7::event {


timer_provider::~timer_provider()
{
    (void)::close(fd_);
}

timer_provider::timer_provider(callback_t&& callback): callback_(std::move(callback))
{
}


result<> timer_provider::setup(c7::usec_t beg, c7::usec_t interval, bool is_abs)
{
    fd_ = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
    if (fd_ == C7_SYSERR) {
	return c7result_err(errno, "timerfd_create() failed");
    }

    ::itimerspec itm;
    itm.it_value.tv_sec  = beg / C7_TIME_S_us;
    itm.it_value.tv_nsec = (beg % C7_TIME_S_us) * 1000;
    itm.it_interval.tv_sec  = interval/ C7_TIME_S_us;
    itm.it_interval.tv_nsec = (interval % C7_TIME_S_us) * 1000;
	
    int flags = is_abs ? TFD_TIMER_ABSTIME : 0;

    if (timerfd_settime(fd_, flags, &itm, nullptr) == C7_SYSERR) {
	return c7result_err(errno, "timerfd_settime(fd:%{})", fd_);
    }

    count_ = (interval == 0) ? 1 : -1UL;
    return c7result_ok();
}


result<int>
timer_provider::manage(monitor& mon,
			  c7::usec_t beg, c7::usec_t interval, callback_t callback,
			  bool is_abs)
{
    auto timer = std::unique_ptr<timer_provider>(new timer_provider(std::move(callback)));
    if (timer == nullptr) {
	return c7result_err("timer_provider() failed");
    }
    if (auto res = timer->setup(beg, interval, is_abs); !res) {
	return c7result_err(std::move(res));
    }
    auto fd = timer->fd();
    if (auto res = mon.manage(std::move(timer)); !res) {
	return c7result_err(std::move(res));
    }
    return c7result_ok(fd);
}


void timer_provider::on_event(monitor& mon, int, uint32_t events)
{
    uint64_t tmo_n;
    if (::read(fd_, &tmo_n, sizeof(tmo_n)) != sizeof(tmo_n)) {
	tmo_n = 0;	// mean error happen
	count_ = 1;	// enforce unmanage after invoking callback
    }
    count_--;
    callback_(fd_, tmo_n);
    if (count_ == 0) {
	mon.unmanage(fd_);
    }
}


} // namespace c7::event
