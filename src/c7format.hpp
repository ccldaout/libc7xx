/*
 * c7format.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1604237671
 */
#ifndef C7_FORMAT_HPP_LOADED__
#define C7_FORMAT_HPP_LOADED__
#include <c7common.hpp>


#include <c7defer.hpp>
#include <c7delegate.hpp>
#include <c7typefunc.hpp>
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

    static constexpr type value{};
};

template <typename T>
struct formatter_tag<T*> {
    typedef formatter_pointer_tag type;

    static constexpr type value{};
};

template <>
struct formatter_tag<std::string> {
    typedef formatter_other_tag type;

    static constexpr type value{};
};

template <>
struct formatter_tag<const char*> {
    typedef formatter_other_tag type;

    static constexpr type value{};
};

template <>
struct formatter_tag<char*> {
    typedef formatter_other_tag type;

    static constexpr type value{};
};


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
	out_ << ssize_t(arg);
    }

    template <typename T>
    void handle_arg(state_t s, T arg, formatter_int_tag) noexcept;

    template <typename T>
    void handle_arg(state_t s, T arg, formatter_float_tag) noexcept;

    template <typename T>
    inline void handle_arg(state_t s, T arg, formatter_pointer_tag) noexcept {
	out_ << arg;
    }

    template <typename T>
    void handle_arg(state_t s, const T& arg, formatter_formattable_tag) noexcept {
	format_traits<T>::convert(out_, ext_, arg);
    }

    template <typename T>
    inline void handle_arg(state_t s, const T& arg, formatter_other_tag) noexcept {
	out_ << arg;
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

    static c7::defer lock();
};


/*----------------------------------------------------------------------------
                                 entry point
----------------------------------------------------------------------------*/

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

template <typename... Args>
inline std::string format(const char *fmt, const Args&... args) noexcept
{
    std::stringstream tmpout;
    formatter formatter(tmpout);
    formatter.parse(fmt, args...);
    return tmpout.str();
}

template <typename... Args>
inline std::string format(const std::string& fmt, const Args&... args) noexcept
{
    return format(fmt.c_str(), args...);
}

template <typename... Args>
inline void p_(const char *fmt, const Args&... args) noexcept
{
    auto defer = formatter::lock();
    formatter formatter(std::cout);
    formatter.parse(fmt, args...);
    std::cout << std::endl;
}

template <typename... Args>
inline void p_(const std::string& fmt, const Args&... args) noexcept
{
    p_(fmt.c_str(), args...);
}

template <typename... Args>
inline void p__(const char *fmt, const Args&... args) noexcept
{
    auto defer = formatter::lock();
    formatter formatter(std::cout);
    formatter.parse(fmt, args...);
}

template <typename... Args>
inline void p__(const std::string& fmt, const Args&... args) noexcept
{
    p__(fmt.c_str(), args...);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7format.hpp
