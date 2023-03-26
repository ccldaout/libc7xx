/*
 * format_r1.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_FORMAT_R1_HPP_LOADED__
#define C7_FORMAT_R1_HPP_LOADED__
#include <c7common.hpp>


#include <c7defer.hpp>
#include <c7format/format_cmn.hpp>
#include <c7strmbuf/strref.hpp>
#include <iomanip>
#include <iostream>
#include <string>


namespace c7::format_r1 {


/*----------------------------------------------------------------------------
                                  formatter
----------------------------------------------------------------------------*/

class formatter {
private:
    enum state_t {
	FINISH, REQ_WIDTH, REQ_PREC, FORMAT_ARG,
    };

    std::ostream& out_;

    // following members are updated by next_format();
    const char *top_;		// point to top of format specification (leading '%')
    const char *cur_;		// point to immediate next character of format specification.
    std::string ext_;		// hold extension if %{xxx[[:]extenstion]} is specified.
    bool require_precision_;	// set true iff '*' is specified as precision.

    state_t next_format(state_t prev_state, bool no_args);

    template <typename T>
    inline void handle_arg(state_t s, const T& arg, formatter_enum_tag) noexcept {
	handle_arg<ssize_t>(s, static_cast<ssize_t>(arg), formatter_int_tag());
    }

    template <typename T>
    void handle_arg(state_t s, T arg, formatter_int_tag) noexcept;

    template <typename T>
    void handle_arg(state_t s, T arg, formatter_int8_tag) noexcept;

    template <typename T>
    void handle_arg(state_t s, T arg, formatter_uint8_tag) noexcept;

    template <typename T>
    void handle_arg(state_t s, T arg, formatter_float_tag) noexcept;

    template <typename T>
    inline void handle_arg(state_t s, T arg, formatter_pointer_tag) noexcept {
	out_ << arg;
    }

    template <typename T>
    void handle_arg(state_t s, const T& arg, formatter_traits_tag) noexcept {
	format_traits<T>::convert(out_, ext_, arg);
    }

    template <typename T>
    void handle_arg(state_t s, const T& arg, formatter_function_tag) noexcept {
	print_type(out_, ext_, arg);
    }

    template <typename T>
    void handle_arg(state_t s, const T& arg, formatter_member_tag) noexcept {
	arg.print(out_, ext_);
    }

    template <typename T>
    void handle_arg(state_t s, const T& arg, formatter_printas_tag) noexcept {
	auto as_value = arg.print_as();
	typedef decltype(as_value) U;
	handle_arg<U>(s, as_value, formatter_tag<U>::value);
    }

    template <typename T>
    inline void handle_arg(state_t s, const T& arg, formatter_operator_tag) noexcept {
	out_ << arg;
    }

    template <typename T>
    inline void handle_arg(state_t s, const T& arg, formatter_error_tag) noexcept {
	static_assert(c7::typefunc::false_v<T>, "Specified type is not formattable.");
    }

    void format_impl(state_t s) noexcept {}

    template <typename Thead, typename... Tbody>
    void format_impl(state_t s, Thead& head, Tbody&... tail) noexcept {
	typedef std::remove_reference_t<std::remove_cv_t<Thead>> U;
	handle_arg<U>(s, head, formatter_tag<U>::value);
	s = next_format(s, c7::typefunc::is_empty_v<Tbody...>);
	format_impl(s, tail...);
    }

public:
    explicit formatter(std::ostream& out): out_(out) {}

    template <typename... Args>
    void parse(const char *fmt, Args&... args) noexcept {
	cur_ = fmt;
	require_precision_ = false;
	auto s = next_format(FORMAT_ARG, c7::typefunc::is_empty_v<Args...>);
	format_impl(s, args...);
    }
};


/*----------------------------------------------------------------------------
                                 entry point
----------------------------------------------------------------------------*/

// add to stream

template <typename... Args>
inline void format(std::ostream& out, const char *fmt, const Args&... args) noexcept
{
    formatter formatter(out);
    formatter.parse(fmt, args...);
}

template <typename... Args>
inline void format(std::ostream& out, const std::string& fmt, const Args&... args) noexcept
{
    format(out, fmt.c_str(), args...);
}

// add to string

template <typename... Args>
inline void format(std::string& str, const char *fmt, const Args&... args) noexcept
{
    auto sb = c7::strmbuf::strref(str);
    auto os = std::basic_ostream(&sb);
    formatter formatter(os);
    formatter.parse(fmt, args...);
}

template <typename... Args>
inline void format(std::string& str, const std::string& fmt, const Args&... args) noexcept
{
    format(str, fmt.c_str(), args...);
}

// new string

template <typename... Args>
inline std::string format(const char *fmt, const Args&... args) noexcept
{
    std::string str;
    format(str, fmt, args...);
    return str;
}

template <typename... Args>
inline std::string format(const std::string& fmt, const Args&... args) noexcept
{
    return format(fmt.c_str(), args...);
}

// std::cout without NL(new line) and with lock

template <typename... Args>
inline void p__(const char *fmt, const Args&... args) noexcept
{
    c7::format_cmn::cout_lock lock;
    formatter formatter(std::cout);
    formatter.parse(fmt, args...);
}

template <typename... Args>
inline void p__(const std::string& fmt, const Args&... args) noexcept
{
    p__(fmt.c_str(), args...);
}

// std::cout without NL(new line) and without lock

template <typename... Args>
inline void p__nolock(const char *fmt, const Args&... args) noexcept
{
    formatter formatter(std::cout);
    formatter.parse(fmt, args...);
}

template <typename... Args>
inline void p__nolock(const std::string& fmt, const Args&... args) noexcept
{
    p__nolock(fmt.c_str(), args...);
}

// std::cout with NL(new line) and with lock

template <typename... Args>
inline void p_(const char *fmt, const Args&... args) noexcept
{
    c7::format_cmn::cout_lock lock;
    formatter formatter(std::cout);
    formatter.parse(fmt, args...);
    std::cout << std::endl;
}

template <typename... Args>
inline void p_(const std::string& fmt, const Args&... args) noexcept
{
    p_(fmt.c_str(), args...);
}


// std::cout with NL(new line) and without lock

template <typename... Args>
inline void p_nolock(const char *fmt, const Args&... args) noexcept
{
    formatter formatter(std::cout);
    formatter.parse(fmt, args...);
    std::cout << std::endl;
}

template <typename... Args>
inline void p_nolock(const std::string& fmt, const Args&... args) noexcept
{
    p_nolock(fmt.c_str(), args...);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::format_r1


#endif // format_r1.hpp
