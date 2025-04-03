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


template <typename FormatItemVec>
void analyze_format(const char *s, FormatItemVec& fmtv)
{
    for (;;) {
	format_item fmt{};
	fmt.flags = (std::ios_base::boolalpha | std::ios_base::right);
	fmt.pref_beg = s;
	fmt.pref_len = 0;
	fmt.ext_beg = nullptr;
	fmt.ext_len = 0;
	fmt.width = 0;
	fmt.prec = 6;
	fmt.padding = ' ';
	fmt.single_fmt = 0;

	const char *p;
	for (;;) {
	    p = std::strchr(s, '%');
	    if (p == nullptr) {
		fmt.pref_len = std::strchr(s, 0) - fmt.pref_beg;
		fmt.type = format_item::format_type::prefonly;
		fmtv.push_back(fmt);
		return;					// FINISH (SUCCESS)
	    }
	    p++;
	    if (*p == '{') {
		fmt.pref_len = p - fmt.pref_beg - 1;
		p++;
		break;
	    }
	    if (*p == '%') {			// %% -> %
		fmt.pref_len = p - fmt.pref_beg;
		fmt.type = format_item::format_type::prefonly;
		fmtv.push_back(fmt);
		fmt.pref_beg = s = p + 1;
		continue;
	    }
	    s = p + 1;
	}
	// p point to next character with "...%{"

	// shortcut
	if (*p == '}') {
	    fmt.type = format_item::format_type::arg;
	    fmtv.push_back(fmt);
	    s = p + 1;
	    continue;
	}

	// conversion flags
	for (;; p++) {
	    switch (*p) {
	    case 0:
		// Invalid format
		fmt.type = format_item::format_type::prefonly;
		fmtv.push_back(fmt);
		return;					// FINISH (ERROR)

	    case '<':
		fmt.set_adjust(std::ios_base::left);
		break;

	    case '=':
		fmt.set_adjust(std::ios_base::internal);
		break;

	    case '>':
		fmt.set_adjust(std::ios_base::right);
		break;

	    case '+':
		fmt.flags |= std::ios_base::showpos;
		break;

	    case '_':
	    case '0':
		fmt.padding = *p;
		break;

	    case '#':
		fmt.flags |= std::ios_base::showbase;
		fmt.set_adjust(std::ios_base::internal);
		break;

	    default:
		goto end_conversion;
	    }
	}
      end_conversion:

	// width
	if (*p == '*') {
	    fmt.type = format_item::format_type::width;
	    fmtv.push_back(fmt);
	    fmt.width = -1;
	    p++;
	} else if (std::isdigit(*p)) {
	    char *e;
	    fmt.width = std::strtol(p, &e, 10);
	    p = e;
	}

	// precision
	if (*p == '.') {
	    p++;
	    if (*p == '*') {
		fmt.type = format_item::format_type::prec;
		fmtv.push_back(fmt);
		fmt.prec = -1;
		p++;
	    } else if (std::isdigit(*p)) {
		char *e;
		fmt.prec = std::strtol(p, &e, 10);
		p = e;
	    }
	}

	// shortcut
	if (*p == '}') {
	    fmt.type = format_item::format_type::arg;
	    fmtv.push_back(fmt);
	    s = p + 1;
	    continue;
	}

	// extenstion
	if (*p == ':') {
	    p++;			// ':' is optional
	}

	auto q = std::strchr(p, '}');
	if (q == nullptr) {
	    // Invalid type specification ('}' is not found)
	    fmt.type = format_item::format_type::prefonly;
	    fmtv.push_back(fmt);
	    return;					// FINISH (ERROR)
	}

	bool ext = true;
	if (p + 1 == q) {
	    fmt.single_fmt = *p;
	    ext = false;
	    switch (*p) {
	    case 'd':
		fmt.flags |= std::ios_base::dec;
		break;
	    case 'o':
		fmt.flags |= std::ios_base::oct;
		break;
	    case 'x':
		fmt.flags |= std::ios_base::hex;
		break;
	    case 'X':
		fmt.flags |= std::ios_base::hex | std::ios_base::uppercase;
		break;
	    case 'f':
		fmt.flags |= std::ios_base::fixed;
		break;
	    case 'e':
		fmt.flags |= std::ios_base::scientific;
		break;
	    case 'E':
		fmt.flags |= std::ios_base::scientific | std::ios_base::uppercase;
		break;
	    case 'g':
		break;
	    case 'G':
		fmt.flags |= std::ios_base::uppercase;
		break;
	    case 'c':
		ext = true;
		break;
	    default:
		fmt.single_fmt = 0;
		ext = true;
	    }
	}
	if (!ext) {
	    fmt.type = format_item::format_type::arg;
	} else {
	    fmt.type = format_item::format_type::arg_ext;
	    fmt.ext_beg = p;
	    fmt.ext_len = q - p;
	}
	fmtv.push_back(fmt);
	s = q + 1;
    }
}

template void analyze_format(const char *s, std::vector<format_item>& fmtv);
template void analyze_format(const char *s, limited_vector<format_item>& fmtv);


/*----------------------------------------------------------------------------
                                  formatter
----------------------------------------------------------------------------*/

const format_item&
formatter::apply_item(const format_item * const fmts, size_t nfmt, size_t& index) noexcept
{
    while (index < nfmt) {
	auto& fmt = fmts[index];
	if (fmt.type == format_item::format_type::arg ||
	    fmt.type == format_item::format_type::arg_ext) {
	    out_.write(fmt.pref_beg, fmt.pref_len);
	    out_.flags(fmt.flags);
	    if (fmt.width != -1) {
		out_.width(fmt.width);
	    }
	    if (fmt.prec != -1) {
		out_.precision(fmt.prec);
	    }
	    out_.fill(fmt.padding);
	} else if (fmt.type == format_item::format_type::prefonly) {
	    out_.write(fmt.pref_beg, fmt.pref_len);
	    index++;
	    continue;
	}
	return fmt;
    }
    out_ << " %{?}";
    static format_item default_item;
    return default_item;
}


void
formatter::apply_item_last(const format_item * const fmts, size_t nfmt, size_t& index) noexcept
{
    while (index < nfmt) {
	auto& fmt = fmts[index];
	out_.write(fmt.pref_beg, fmt.pref_len);
	if (fmt.type == format_item::format_type::arg ||
	    fmt.type == format_item::format_type::arg_ext) {
	    out_ << "%{}";
	}
	index++;
    }
}


template <typename T>
void formatter::handle_inttype_ext(const format_item& fmt, T arg) noexcept
{
    if (fmt.type == format_item::format_type::arg_ext) {
	if (fmt.single_fmt == 'c') {
	    out_ << static_cast<char>(arg);
	} else {
	    format_traits<ssize_t>::convert(out_, ext(fmt), static_cast<ssize_t>(arg));
	}
    } else if (fmt.type == format_item::format_type::width) {
	out_.width(arg);
    } else if (fmt.type == format_item::format_type::prec) {
	out_.precision(arg);
    }
}


template void formatter::handle_inttype_ext(const format_item& fmt, bool arg);
template void formatter::handle_inttype_ext(const format_item& fmt, char arg);
template void formatter::handle_inttype_ext(const format_item& fmt, signed char arg);
template void formatter::handle_inttype_ext(const format_item& fmt, unsigned char arg);
template void formatter::handle_inttype_ext(const format_item& fmt, short arg);
template void formatter::handle_inttype_ext(const format_item& fmt, unsigned short arg);
template void formatter::handle_inttype_ext(const format_item& fmt, int arg);
template void formatter::handle_inttype_ext(const format_item& fmt, unsigned int arg);
template void formatter::handle_inttype_ext(const format_item& fmt, long arg);
template void formatter::handle_inttype_ext(const format_item& fmt, unsigned long arg);


/*----------------------------------------------------------------------------
                              lock for std::cout
----------------------------------------------------------------------------*/

uint64_t cout_lock::lock_;


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

class initialize {
public:
    initialize() {
	std::ios::sync_with_stdio(false);
    }
private:
    static initialize init_;
};
initialize initialize::init_;


} // namespace c7::format_cmn

