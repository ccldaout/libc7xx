/*
 * c7format.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __C7_FORMAT_HPP_LOADED__
#define __C7_FORMAT_HPP_LOADED__
#include "c7common.hpp"


#include "c7defer.hpp"
#include "c7delegate.hpp"
#include <iomanip>
#include <iostream>	// cout
#include <string>
#include <type_traits>


namespace c7 {


inline std::ostream& operator<<(std::ostream& out, const std::stringstream& ss)
{
    auto s = ss.str();
    return out.write(s.c_str(), s.size());
}


/*----------------------------------------------------------------------------
                                format traits
----------------------------------------------------------------------------*/

template <>
struct format_traits<ssize_t> {
    static c7::delegate<bool, std::ostream&, const std::string&, ssize_t> converter;
    static void convert(std::ostream& out, const std::string& format, ssize_t arg);
};

template <>
struct format_traits<com_status> {
    static void convert(std::ostream& out, const std::string& format, com_status arg);
};


/*----------------------------------------------------------------------------
                                type functions
----------------------------------------------------------------------------*/

// check type is formattable or not.

struct is_formattable_impl {
    template <typename T>
    static auto check(T*) -> decltype(
	format_traits<T>::convert,
	std::true_type());

    template <typename T>
    static auto check(...) -> std::false_type;
};

template <typename T>
struct is_formattable:
	public decltype(is_formattable_impl::check<T>(nullptr)) {};

template <typename T>
inline constexpr bool is_formattable_v = is_formattable<T>::value;


// tag dispatch

struct formatter_int_tag {};
struct formatter_float_tag {};
struct formatter_string_tag {};
struct formatter_pointer_tag {};
struct formatter_formattable_tag {};
struct formatter_enum_tag {};
struct formatter_other_tag {};

template <typename T>
struct formatter_tag {
    typedef typename
    std::conditional_t<
	std::is_integral_v<T>,
	formatter_int_tag,
	std::conditional_t<
	    std::is_floating_point_v<T>,
	    formatter_float_tag,
	    std::conditional_t<
		is_formattable_v<T>,
		formatter_formattable_tag,
		std::conditional_t<
		    std::is_enum_v<T>,
		    formatter_enum_tag,
		    formatter_other_tag>>>
	> type;

    static constexpr type value() {
	return type();
    }
};

template <typename T>
struct formatter_tag<T*> {
    typedef formatter_pointer_tag type;

    static constexpr type value() {
	return type();
    }
};

template <>
struct formatter_tag<std::string> {
    typedef formatter_string_tag type;

    static constexpr type value() {
	return type();
    }
};

template <>
struct formatter_tag<const char*> {
    typedef formatter_string_tag type;

    static constexpr type value() {
	return type();
    }
};

template <>
struct formatter_tag<char*> {
    typedef formatter_string_tag type;

    static constexpr type value() {
	return type();
    }
};


/*----------------------------------------------------------------------------
                                  formatter
----------------------------------------------------------------------------*/

class formatter {
private:
    enum state_t {
	FINISH, REQ_WIDTH, REQ_PREC, FORMAT_ARG,
    };

    enum result_t {
	OK, ERR_TOOFEW, ERR_TOOMANY, ERR_REQUIRE_INT,
    };

    std::ostream* out_;

    // following members are updated by next_format();
    const char *top_;		// point to top of format specification (leading '%')
    const char *cur_;		// point to immediate next character of format specification.
    std::string ext_;		// hold extension if %{xxx[[:]extenstion]} is specified.
    bool require_precision_;	// set true iff '*' is specified as precision.

    state_t next_format();

    template <typename T>
    inline result_t handle_arg(state_t s, const T& arg, formatter_enum_tag) {
	*out_ << ssize_t(arg);
	return OK;
    }

    template <typename T>
    result_t handle_arg(state_t s, T arg, formatter_int_tag) {
	switch (s) {
	  case REQ_WIDTH:
	    *out_ << std::setw(arg);
	    return OK;

	  case REQ_PREC:
	    *out_ << std::setprecision(arg);
	    return OK;

	  default:
	    switch (ext_[0]) {
	      case 'o':
		*out_ << std::oct;
		break;
	      case 'x':
		*out_ << std::hex;
		break;
	      case 0:
	      case 'd':
		*out_ << std::dec;
		break;
	      default:
		format_traits<ssize_t>::convert(*out_, ext_, static_cast<ssize_t>(arg));
		return OK;
	    }
	    *out_ << arg;
	    return OK;
	}
    }

    template <typename T>
    result_t handle_arg(state_t s, T arg, formatter_float_tag) {
	switch (ext_[0]) {
	  case 'e':
	    *out_ << std::scientific;
	    break;
	  case 'f':
	    *out_ << std::fixed;
	    break;
	  default:
	    *out_ << std::defaultfloat;
	}
	*out_ << arg;
	return OK;
    }

    template <typename T>
    inline result_t handle_arg(state_t s, const T& arg, formatter_string_tag) {
	*out_ << arg;
	return OK;
    }

    template <typename T>
    inline result_t handle_arg(state_t s, T arg, formatter_pointer_tag) {
	*out_ << arg;
	return OK;
    }

    template <typename T>
    result_t handle_arg(state_t s, const T& arg, formatter_formattable_tag) {
	// User defined format_traits<T>::convert() may call c7::format
	safe_call([&](auto& ext){
		auto out_save = out_;
		std::stringstream tmpout;
		format_traits<T>::convert(tmpout, ext, arg);
		out_ = out_save;
		*out_ << tmpout.str();
	    });
	return OK;
    }

    template <typename T>
    inline result_t handle_arg(state_t s, const T& arg, formatter_other_tag) {
	// User defined operator<<() may call c7::format
	safe_call([&](auto){ *out_ << arg; });
	return OK;
    }

    void safe_call(std::function<void(std::string&)> f) {
	auto cur_save = cur_;
	auto top_save = top_;
	auto ext_save = std::move(ext_);
	f(ext_save);
	cur_ = cur_save;
	top_ = top_save;
	ext_ = std::move(ext_save);
    }

    std::tuple<int, result_t>
    format_impl(state_t s, int prm_num) {
	return { prm_num, (s == FINISH) ? OK : ERR_TOOFEW };
    }

    template <typename Thead, typename... Tbody>
    std::tuple<int, result_t>
    format_impl(state_t s, int prm_pos, Thead& head, Tbody&... tail) {
	if (s == FINISH)
	    return { prm_pos, ERR_TOOMANY };

	if (s != FORMAT_ARG && !std::is_integral_v<Thead>)
	    return { prm_pos, ERR_REQUIRE_INT };

	typedef std::remove_reference_t<std::remove_cv_t<Thead>> U;
	auto r = handle_arg<U>(s, head, formatter_tag<U>::value());
	if (r != OK)
	    return { prm_pos, r };

	if (s == REQ_WIDTH)
	    s = (require_precision_ ? REQ_PREC : FORMAT_ARG);
	else if (s == REQ_PREC)
	    s = FORMAT_ARG;
	else
	    s = next_format();

	return format_impl(s, prm_pos+1, tail...);
    }

    void put_error(int prm_pos, result_t err);

public:
    template <typename... Args>
    void parse(std::ostream& out, const std::string& fmt, Args&... args) {
	out_ = &out;
	cur_ = fmt.c_str();
	require_precision_ = false;
	auto s = next_format();
	auto [prm_pos, err] = format_impl(s, 1, args...);
	if (err != OK)
	    put_error(prm_pos, err);
    }

    static c7::defer lock();
};


#undef  __C7_EXTERN 
#define __C7_EXTERN(T)							\
    extern template formatter::result_t formatter::handle_arg<T>(state_t s, T arg, formatter_int_tag)
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
    extern template formatter::result_t formatter::handle_arg<T>(state_t s, T arg, formatter_float_tag)
__C7_EXTERN(float);
__C7_EXTERN(double);

#undef __C7_EXTERN


extern thread_local formatter __formatter;


/*----------------------------------------------------------------------------
                                 entry point
----------------------------------------------------------------------------*/

template <typename... Args>
inline void format(std::ostream& out, const std::string& fmt, const Args&... args)
{
    __formatter.parse(out, fmt, args...);
}

template <typename... Args>
inline std::string format(const std::string& fmt, const Args&... args)
{
    std::stringstream tmpout;
    __formatter.parse(tmpout, fmt, args...);
    return tmpout.str();
}

template <typename... Args>
inline void p_(const std::string& fmt, const Args&... args)
{
    auto defer = formatter::lock();
    __formatter.parse(std::cout, fmt, args...);
    std::cout << std::endl;
}

template <typename... Args>
inline void p__(const std::string& fmt, const Args&... args)
{
    auto defer = formatter::lock();
    __formatter.parse(std::cout, fmt, args...);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7format.hpp
