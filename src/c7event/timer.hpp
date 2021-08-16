/*
 * c7event/timer.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_TIMER_HPP_LOADED__
#define C7_EVENT_TIMER_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/monitor.hpp>


namespace c7::event {


class timer_provider: public provider_interface {
public:
    using callback_t = std::function<void(int, uint64_t)>;	// fd, #timeout

    ~timer_provider() override;
    int fd() override { return fd_; }
    void on_event(monitor& mon, int, uint32_t events) override;

    static result<int> manage(monitor& mon,
				 c7::usec_t beg, c7::usec_t interval, callback_t callback,
				 bool is_abs);
private:
    int fd_ = C7_SYSERR;
    uint64_t count_ = 0;
    callback_t callback_;

    explicit timer_provider(callback_t&&);
    result<> setup(c7::usec_t beg, c7::usec_t interval, bool is_abs);
};


inline result<int>
timer_start(c7::usec_t beg, c7::usec_t interval,
	    std::function<void(int, uint64_t)> callback)
{
    return timer_provider::manage(default_event_monitor(),
				     beg, interval, std::move(callback), false);
}

inline result<int>
timer_start(monitor& mon,
	    c7::usec_t beg, c7::usec_t interval,
	    std::function<void(int, uint64_t)> callback)
{
    return timer_provider::manage(mon, beg, interval, std::move(callback), false);
}

inline result<int>
timer_start_abs(c7::usec_t beg, c7::usec_t interval,
		std::function<void(int, uint64_t)> callback)
{
    return timer_provider::manage(default_event_monitor(),
				     beg, interval, std::move(callback), true);
}

inline result<int>
timer_start_abs(monitor& mon,
		c7::usec_t beg, c7::usec_t interval,
		std::function<void(int, uint64_t)> callback)
{
    return timer_provider::manage(mon, beg, interval, std::move(callback), true);
}


} // c7::event


#endif // c7event/timer.hpp
