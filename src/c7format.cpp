/*
 * c7format.cpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include "c7format.hpp"
#include "c7thread.hpp"
#include <cctype>
#include <cstring>
#include <time.h>	// ::localtime_r


namespace c7 {


thread_local formatter __formatter;
static c7::thread::spinlock __global_lock;


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

void format_traits<com_status>::convert(std::ostream& out, const std::string& format, com_status arg)
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
                          parse format specification
----------------------------------------------------------------------------*/

struct format_info {
    enum position_t { RIGHT, LEFT, CENTER };
    position_t pos = RIGHT;
    bool sign = false;
    char padding = ' ';
    bool alt = false;		// true: showbase / boolalpha
    int width = 0;		// -1: from parameter
    int prec = 6;		// -1: from parameter
};

static inline auto position(format_info::position_t pos)
{
    switch (pos) {
      case format_info::RIGHT:
	return std::right;

      case format_info::LEFT:
	return std::left;

      case format_info::CENTER:
      default:
	return std::internal;
    }
}

static inline auto sign(bool s)
{
    return s ? std::showpos : std::noshowpos;
}

static inline auto alternative(bool a)
{
    return a ? std::showbase : std::noshowbase;
}

formatter::state_t formatter::next_format()
{
    auto& out = *out_;

    // reset characteristics
    format_info fm;
    require_precision_ = false;
    ext_.clear();

    // put leading string
    for (;;) {
	auto p = std::strchr(cur_, '%');
	if (p == nullptr) {
	    out << cur_;
	    top_ = cur_;
	    return FINISH;
	}
	out.write(cur_, p - cur_);	// exclude 1st '%' 
	cur_ = p + 1;
	if (*cur_ == '{')
	    break;
	if (*cur_ == '%')		// 2nd '%'
	    cur_++;
	out << '%';			// put only one '%'
	cur_++;
    }
    top_ = cur_ - 1;			// cur_ posint '{', top_ point '%'

    // conversion flags
    for (cur_++;; cur_++) {
	switch (*cur_) {
	  case 0:
	    out << "... Invalid format <" << top_ << ">\n";
	    return FINISH;

	  case '<':
	    fm.pos = format_info::LEFT;
	    break;

	  case '=':
	    fm.pos = format_info::CENTER;
	    break;

	  case '>':
	    fm.pos = format_info::RIGHT;
	    break;

	  case '+':
	    fm.sign = true;
	    break;

	  case '_':
	  case '0':
	    fm.padding = *cur_;
	    break;

	  case '#':
	    fm.alt = true;
	    break;

	  default:
	    goto end_conversion;
	}
    }
  end_conversion:

    // width
    if (*cur_ == '*') {
	fm.width = -1;
	cur_++;
    } else {
	char *e;
	auto w = std::strtol(cur_, &e, 10);
	if (cur_ != e) {
	    fm.width = w;
	    cur_ = e;
	}
    }

    // precision
    if (*cur_ == '.') {
	cur_++;
	if (*cur_ == '*') {
	    fm.prec = -1;
	    cur_++;
	    require_precision_ = true;
	} else {
	    char *e;
	    auto w = std::strtol(cur_, &e, 10);
	    if (cur_ != e) {
		fm.prec = w;
		cur_ = e;
	    }
	}
    }

    // extenstion
    if (*cur_ == ':')
	cur_++;			// ':' is optional
    
    auto q = std::strchr(cur_, '}');
    if (q == nullptr) {
	out << "... Invalid type specification ('}' is not found) <" << top_ << ">\n";
	return FINISH;
    }
    ext_.append(cur_, q - cur_);

    // At this point, cur_ point to first character of extenstion.
    cur_ = q + 1;		// next position

    // apply format_info to io manipulations.
    out << std::boolalpha;
    out << position(fm.pos);
    out << sign(fm.sign);
    out << std::setfill(fm.padding);
    out << alternative(fm.alt);
    if (fm.width != -1)
	out << std::setw(fm.width);
    if (fm.prec != -1)
	out << std::setprecision(fm.prec);

    // determine next action
    if (fm.width == -1)
	return REQ_WIDTH;
    if (fm.prec == -1)
	return REQ_PREC;
    return FORMAT_ARG;
}

void formatter::put_error(int prm_pos, formatter::result_t err)
{
    struct table_t {
	formatter::result_t err;
	const char *str;
    } table[] = {
	{ ERR_TOOFEW, "Too few parameters" },
	{ ERR_TOOMANY, "Too many parameters" },
	{ ERR_REQUIRE_INT, "Integer is requirted here" },
    };

    for (int i = 0; i < c7_numberof(table); i++) {
	if (table[i].err == err) {
	    *out_ << "\nERROR: " << prm_pos << "th parameter: "
		  << table[i].str << ": " << std::string(top_, cur_ - top_) << std::endl;
	    return;
	}
    }

    *out_ << "\nERROR: " << prm_pos << "th parameter: error:" << err << std::endl;
}


c7::defer formatter::lock()
{
    return __global_lock.lock();
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

#undef  __C7_EXTERN 
#define __C7_EXTERN(T)							\
    template formatter::result_t formatter::handle_arg<T>(state_t s, T arg, formatter_int_tag)
__C7_EXTERN(bool);
__C7_EXTERN(char);
__C7_EXTERN(signed char);
__C7_EXTERN(unsigned char);
__C7_EXTERN(short int);
__C7_EXTERN(unsigned short int);
__C7_EXTERN(int);
__C7_EXTERN(unsigned int);
__C7_EXTERN(long);
__C7_EXTERN(unsigned long);

#undef  __C7_EXTERN 
#define __C7_EXTERN(T)							\
    template formatter::result_t formatter::handle_arg<T>(state_t s, T arg, formatter_float_tag)
__C7_EXTERN(float);
__C7_EXTERN(double);

#undef __C7_EXTERN


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7
