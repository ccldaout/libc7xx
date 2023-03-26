/*
 * format_cmn.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <pthread.h>
#include <cctype>
#include <cstring>
#include <iomanip>	// std::put_time
#include <time.h>	// ::localtime_r
#include <c7format/format_cmn.hpp>


namespace c7 {


/*----------------------------------------------------------------------------
                              special formatter
----------------------------------------------------------------------------*/

// for ssize_t

c7::delegate<bool, std::ostream&, const std::string&, ssize_t>
format_traits<ssize_t>::converter;

void format_traits<ssize_t>::convert(std::ostream& out, const std::string& format, ssize_t arg)
{
    if (converter(out, format, arg))
	return;

    const char* p = format.c_str();
    if (*p == 'b') {
	size_t m = static_cast<size_t>(arg);
	int w = 0;
	if (std::isdigit(int(*++p)))
	    w = std::strtol(p, nullptr, 10);
	else {
	    for (w = 64; w > 1; w--) {
		if (((1UL << (w-1)) & m) != 0)
		    break;
	    }
	}
	for (w--; w >= 0; w--) {
	    out << (((1UL << w) & m) ? '1' : '0');
	}
    } else if (*p == 't' || *p == 'T') {
	bool us_type = (*p == 't');
	time_t tv;
	ssize_t us;

	if (us_type) {
	    us = arg % C7_TIME_S_us;
	    tv = arg / C7_TIME_S_us;
	} else
	    tv = arg;

	if (*++p == 0)
	    p = "%H:%M:%S";

	::tm tmbuf;
	(void)::localtime_r(&tv, &tmbuf);
	out << std::put_time(&tmbuf, p);

	if (us_type) {
	    out << "." << std::setw(6) << std::setfill('0') << std::right << std::dec << us;
	}
    }
}

// for com_status

void print_type(std::ostream& out, const std::string& format, com_status arg)
{
    switch (arg) {
      case com_status::OK:
	out << "OK(" << int(arg) << ")";
	break;

      case com_status::CLOSED:
	out << "CLOSED(" << int(arg) << ")";
	break;

      case com_status::TIMEOUT:
	out << "TIMEOUT(" << int(arg) << ")";
	break;

      case com_status::ERROR:
	out << "ERROR(" << int(arg) << ")";
	break;

      case com_status::ABORT:
	out << "ABORT(" << int(arg) << ")";
	break;

      default:
	out << "?(" << int(arg) << ")";
	break;
    }
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


namespace c7::format_cmn {


class initialize {
public:
    initialize() {
	std::ios::sync_with_stdio(false);
    }
private:
    static initialize init_;
};
initialize initialize::init_;


/*----------------------------------------------------------------------------
                              lock for std::cout
----------------------------------------------------------------------------*/

uint64_t cout_lock::lock_;


} // namespace c7::format_cmn

