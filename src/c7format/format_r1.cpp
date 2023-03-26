/*
 * format_r1.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <cstring>
#include <c7format/format_r1.hpp>


namespace c7::format_r1 {


/*----------------------------------------------------------------------------
                          parse format specification
----------------------------------------------------------------------------*/

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
    struct format_info {
	std::ios_base::fmtflags flags = std::ios_base::boolalpha | std::ios_base::right;
	char padding = ' ';
	int width = 0;
	int prec = 6;
	state_t next_act = FORMAT_ARG;
	void set_adjust(std::ios_base::fmtflags adj) {
	    flags &= ~(std::ios_base::left|std::ios_base::internal|std::ios_base::right);
	    flags |= adj;
	}
    } fm;
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
	if (*cur_ == '{') {
	    break;
	}
	if (*cur_ == '%') {		// 2nd '%'
	    cur_++;
	}
	out_ << '%';			// put only one '%'
    }
    top_ = cur_ - 1;			// cur_ point '{', top_ point '%'

    // shortcut
    if (cur_[1] == '}') {
	out_.flags(fm.flags);
	cur_ += 2;
	return FORMAT_ARG;
    }

    // conversion flags
    for (cur_++;; cur_++) {
	switch (*cur_) {
	case 0:
	    out_ << "... Invalid format <" << top_ << ">\n";
	    return FINISH;

	case '<':
	    fm.set_adjust(std::ios_base::left);
	    break;

	case '=':
	    fm.set_adjust(std::ios_base::internal);
	    break;

	case '>':
	    fm.set_adjust(std::ios_base::right);
	    break;

	case '+':
	    fm.flags |= std::ios_base::showpos;
	    break;

	case '_':
	case '0':
	    fm.padding = *cur_;
	    break;

	case '#':
	    fm.flags |= std::ios_base::showbase;
	    fm.set_adjust(std::ios_base::internal);
	    break;

	default:
	    goto end_conversion;
	}
    }
  end_conversion:

    // width
    if (*cur_ == '*') {
	fm.next_act = REQ_WIDTH;
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
	    require_precision_ = true;
	    if (fm.next_act == FORMAT_ARG) {
		fm.next_act = REQ_PREC;
	    }
	    cur_++;
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
    if (*cur_ == ':') {
	cur_++;			// ':' is optional
    }

    auto q = std::strchr(cur_, '}');
    if (q == nullptr) {
	out_ << "... Invalid type specification ('}' is not found) <" << top_ << ">\n";
	return FINISH;
    }
    ext_.append(cur_, q - cur_);

    // At this point, cur_ point to first character of extenstion.
    cur_ = q + 1;		// next position

    // apply io manipulations.
    out_.flags(fm.flags);
    out_.width(fm.width);
    out_.precision(fm.prec);
    out_.fill(fm.padding);

    // next action
    return fm.next_act;
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
	if (ext_.size() == 0) {
	    out_ << std::dec << arg;
	} else if (ext_ == "x") {
	    out_ << std::hex << arg;
	} else if (ext_ == "d") {
	    out_ << std::dec << arg;
	} else if (ext_ == "o") {
	    out_ << std::oct << arg;
	} else if (ext_ == "c") {
	    out_ << static_cast<char>(arg);
	} else {
	    format_traits<ssize_t>::convert(out_, ext_, static_cast<ssize_t>(arg));
	}
    }
}

template <typename T>
void formatter::handle_arg(state_t s, T arg, formatter_int8_tag) noexcept
{
    if (ext_.empty()) {
	handle_arg(s, arg, formatter_int_tag{});
    } else {
	handle_arg(s, static_cast<int>(arg), formatter_int_tag{});
    }
}

template <typename T>
void formatter::handle_arg(state_t s, T arg, formatter_uint8_tag) noexcept
{
    if (ext_.empty()) {
	handle_arg(s, arg, formatter_int_tag{});
    } else {
	handle_arg(s, static_cast<unsigned int>(arg), formatter_int_tag{});
    }
}

template <typename T>
void formatter::handle_arg(state_t s, T arg, formatter_float_tag) noexcept
{
    switch (ext_[0]) {
    case 'f':
	out_ << std::fixed;
	break;
    case 'e':
	out_ << std::scientific;
	break;
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
    template void formatter::handle_arg<T>(state_t s, T arg, formatter_int8_tag)
__C7_EXTERN(char);
__C7_EXTERN(signed char);

template void formatter::handle_arg<unsigned char>(state_t s, unsigned char arg, formatter_uint8_tag);

#undef  __C7_EXTERN 
#define __C7_EXTERN(T)							\
    template void formatter::handle_arg<T>(state_t s, T arg, formatter_float_tag)
__C7_EXTERN(float);
__C7_EXTERN(double);

#undef __C7_EXTERN


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::format_r1
