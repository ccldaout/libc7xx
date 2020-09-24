/*
 * c7format.cpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7format.hpp>
#include <c7thread.hpp>
#include <cctype>
#include <cstring>
#include <time.h>	// ::localtime_r


namespace c7 {


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

formatter::state_t formatter::next_format(state_t prev_state, bool no_args)
{
    if (no_args) {
	out_ << cur_;
	return FINISH;
    }

    switch (prev_state) {
    case FINISH:
	out_ << ' ';
	return FINISH;
    case REQ_WIDTH:
	return (require_precision_ ? REQ_PREC : FORMAT_ARG);
    case REQ_PREC:
	return FORMAT_ARG;
    default:
	break;
    }

    // reset characteristics
    format_info fm;
    require_precision_ = false;
    ext_.clear();

    // put leading string
    for (;;) {
	auto p = std::strchr(cur_, '%');
	if (p == nullptr) {
	    out_ << cur_;
	    top_ = cur_ = std::strchr(cur_, 0);
	    if (!no_args) {
		out_ << " ... Too many args: ";
	    }
	    return FINISH;
	}
	out_.write(cur_, p - cur_);	// exclude 1st '%' 
	cur_ = p + 1;
	if (*cur_ == '{')
	    break;
	if (*cur_ == '%')		// 2nd '%'
	    cur_++;
	out_ << '%';			// put only one '%'
	cur_++;
    }
    top_ = cur_ - 1;			// cur_ posint '{', top_ point '%'

    // conversion flags
    for (cur_++;; cur_++) {
	switch (*cur_) {
	  case 0:
	    out_ << "... Invalid format <" << top_ << ">\n";
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
	out_ << "... Invalid type specification ('}' is not found) <" << top_ << ">\n";
	return FINISH;
    }
    ext_.append(cur_, q - cur_);

    // At this point, cur_ point to first character of extenstion.
    cur_ = q + 1;		// next position

    // apply format_info to io manipulations.
    out_ << std::boolalpha;
    out_ << position(fm.pos);
    out_ << sign(fm.sign);
    out_ << std::setfill(fm.padding);
    out_ << alternative(fm.alt);
    if (fm.width != -1)
	out_ << std::setw(fm.width);
    if (fm.prec != -1)
	out_ << std::setprecision(fm.prec);

    // determine next action
    if (fm.width == -1)
	return REQ_WIDTH;
    if (fm.prec == -1)
	return REQ_PREC;
    return FORMAT_ARG;
}

c7::defer formatter::lock()
{
    return __global_lock.lock();
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

template <typename T>
void formatter::handle_arg(state_t s, T arg, formatter_int_tag) noexcept
{
    switch (s) {
    case REQ_WIDTH:
	out_ << std::setw(arg);
	return;

    case REQ_PREC:
	out_ << std::setprecision(arg);
	return;

    default:
	switch (ext_[0]) {
	case 'o':
	    out_ << std::oct;
	    break;
	case 'x':
	    out_ << std::hex;
	    break;
	case 0:
	case 'd':
	    out_ << std::dec;
	    break;
	default:
	    format_traits<ssize_t>::convert(out_, ext_, static_cast<ssize_t>(arg));
	    return;
	}
	out_ << arg;
    }
}

template <typename T>
void formatter::handle_arg(state_t s, T arg, formatter_float_tag) noexcept
{
    switch (ext_[0]) {
    case 'e':
	out_ << std::scientific;
	break;
    case 'f':
	out_ << std::fixed;
	break;
    case 'g':
    default:
	out_ << std::defaultfloat;
    }
    out_ << arg;
}


#undef  __C7_EXTERN 
#define __C7_EXTERN(T)							\
    template void formatter::handle_arg<T>(state_t s, T arg, formatter_int_tag)
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
    template void formatter::handle_arg<T>(state_t s, T arg, formatter_float_tag)
__C7_EXTERN(float);
__C7_EXTERN(double);

#undef __C7_EXTERN


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7
