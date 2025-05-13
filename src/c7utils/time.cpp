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
                                  make_usec
----------------------------------------------------------------------------*/

make_usec::make_usec(): tmbuf_(), usec_()
{
    tmbuf_.tm_mday = 1;
}

make_usec& make_usec::now()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    usec_ = tv.tv_usec;
    tmbuf_ = *::localtime(&tv.tv_sec);
    return *this;
}

make_usec& make_usec::time_us(c7::usec_t ustv)
{
    time_t tv_sec = ustv / C7_TIME_S_us;
    usec_ =         ustv % C7_TIME_S_us;
    tmbuf_ = *::localtime(&tv_sec);
    return *this;
}

make_usec& make_usec::time_s(time_t stv)
{
    usec_ = 0;
    tmbuf_ = *::localtime(&stv);
    return *this;
}

make_usec& make_usec::tmbuf(struct tm& tmbuf)
{
    usec_ = 0;
    tmbuf_ = tmbuf;
    return *this;
}

make_usec& make_usec::year(int year)
{
    tmbuf_.tm_year = year - 1900;
    return *this;
}

make_usec& make_usec::month(int mon_1)
{
    tmbuf_.tm_mon = mon_1 - 1;
    return *this;
}

make_usec& make_usec::mday(int mday_1)
{
    tmbuf_.tm_mday = mday_1;
    return *this;
}

make_usec& make_usec::hour(int hour)
{
    tmbuf_.tm_hour = hour;
    return *this;
}

make_usec& make_usec::min(int min)
{
    tmbuf_.tm_min = min;
    return *this;
}

make_usec& make_usec::sec(int sec)
{
    tmbuf_.tm_sec = sec;
    return *this;
}

c7::usec_t make_usec::make()
{
    c7::usec_t stv = mktime(&tmbuf_);
    return (stv * C7_TIME_S_us) + usec_;
}


/*----------------------------------------------------------------------------
                               single functions
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
