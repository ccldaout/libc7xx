/*
 * c7event/timer.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_TIMER_HPP_LOADED_
#define C7_EVENT_TIMER_HPP_LOADED_
#include <c7common.hpp>


#include <c7event/monitor.hpp>


namespace c7::event {


struct timer_fd_tag {};
using timer_fd_t = simple_wrap<int, timer_fd_tag>;


class timer_provider: public provider_interface {
public:
    using callback_t = std::function<void(timer_fd_t, uint64_t)>;	// fd, #timeout

    ~timer_provider() override;
    int fd() override { return fd_; }
    void on_event(monitor& mon, int, uint32_t events) override;
    void on_unmanage(monitor& mon, int) override;

    static result<timer_fd_t> manage(monitor& mon,
				     c7::usec_t beg, c7::usec_t interval, callback_t callback,
				     bool is_abs);
private:
    int fd_ = C7_SYSERR;
    uint64_t count_ = 0;
    callback_t callback_;

    explicit timer_provider(callback_t&&);
    result<> setup(c7::usec_t beg, c7::usec_t interval, bool is_abs);
};


inline result<timer_fd_t>
timer_start(c7::usec_t beg, c7::usec_t interval,
	    std::function<void(timer_fd_t, uint64_t)> callback)
{
    return timer_provider::manage(default_event_monitor(),
				     beg, interval, std::move(callback), false);
}

inline result<timer_fd_t>
timer_start(monitor& mon,
	    c7::usec_t beg, c7::usec_t interval,
	    std::function<void(timer_fd_t, uint64_t)> callback)
{
    return timer_provider::manage(mon, beg, interval, std::move(callback), false);
}

inline result<timer_fd_t>
timer_start_abs(c7::usec_t beg, c7::usec_t interval,
		std::function<void(timer_fd_t, uint64_t)> callback)
{
    return timer_provider::manage(default_event_monitor(),
				     beg, interval, std::move(callback), true);
}

inline result<timer_fd_t>
timer_start_abs(monitor& mon,
		c7::usec_t beg, c7::usec_t interval,
		std::function<void(timer_fd_t, uint64_t)> callback)
{
    return timer_provider::manage(mon, beg, interval, std::move(callback), true);
}


} // c7::event


#endif // c7event/timer.hpp
