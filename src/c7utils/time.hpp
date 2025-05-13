/*
 * c7utils/time.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_UTILS_TIME_HPP_LOADED_
#define C7_UTILS_TIME_HPP_LOADED_
#include <c7common.hpp>


#include <time.h>
#include <sys/time.h>	// timespec


namespace c7 {


class make_usec {
public:
    make_usec();
    make_usec& now();
    make_usec& time_us(c7::usec_t);
    make_usec& time_s(time_t);
    make_usec& tmbuf(struct tm& tmbuf);
    make_usec& year(int year);
    make_usec& month(int mon_1);	// Jan:1, ..., Dec:12
    make_usec& mday(int mday_1);
    make_usec& hour(int hour);
    make_usec& min(int min);
    make_usec& sec(int sec);
    c7::usec_t make();

private:
    struct tm tmbuf_;
    c7::usec_t usec_;
};

c7::usec_t time_us();

c7::usec_t sleep_us(c7::usec_t duration);

inline c7::usec_t sleep_ms(c7::usec_t duration_ms)
{
    return sleep_us(duration_ms * 1000) / 1000;
}

::timespec *timespec_from_duration(c7::usec_t duration, c7::usec_t reftime = 0);

inline ::timespec *mktimespec(c7::usec_t duration, c7::usec_t reftime = 0)
{
    return (duration < 0) ? nullptr : timespec_from_duration(duration, reftime);
}


} // namespace c7


#endif // c7utils/time.hpp
