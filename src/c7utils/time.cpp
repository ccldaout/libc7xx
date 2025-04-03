/*
 * c7utils/time.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <time.h>
#include <algorithm>
#include <c7utils/time.hpp>


namespace c7 {


/*----------------------------------------------------------------------------
                                     time
----------------------------------------------------------------------------*/

c7::usec_t time_us()
{
    ::timeval tv;
    (void)::gettimeofday(&tv, nullptr);
    return ((c7::usec_t)tv.tv_sec) * C7_TIME_S_us + tv.tv_usec;
}


c7::usec_t sleep_us(c7::usec_t duration)
{
    ::timespec req, rem;
    req.tv_sec  =  duration / C7_TIME_S_us;
    req.tv_nsec = (duration % C7_TIME_S_us) * 1000;
    if (::nanosleep(&req, &rem) == C7_SYSERR) {
	return C7_SYSERR;
    }
    duration = (rem.tv_sec * C7_TIME_S_us) + (rem.tv_nsec + 500) / 1000;
    return std::max(duration, (c7::usec_t)0);
}


::timespec *timespec_from_duration(c7::usec_t duration, c7::usec_t reftime)
{
    static thread_local ::timespec abstime;
    if (reftime == 0)
	reftime = c7::time_us();
    reftime += duration;
    abstime.tv_sec =   reftime / C7_TIME_S_us;
    abstime.tv_nsec = (reftime % C7_TIME_S_us) * 1000;
    return &abstime;
}


} // namespace c7
