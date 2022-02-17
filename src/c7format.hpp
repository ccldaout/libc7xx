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
                                type functions
----------------------------------------------------------------------------*/

// check type is formattable or not.

struct formatter_has_traits_impl {
    template <typename T>
    static auto check(T*) -> decltype(
	format_traits<T>::convert,
	std::true_type());

    template <typename T>
    static auto check(...) -> std::false_type;
};
template <typename T>
struct formatter_has_traits:
	public decltype(formatter_has_traits_impl::check<T>(nullptr)) {};


// check type has print_type function or not.

struct formatter_has_function_impl {
    template <typename T>
    static auto check(const T* o) -> decltype(
	print_type(std::cout, std::string{}, *o),
	std::true_type{});

    template <typename T>
    static auto check(...) -> std::false_type;
};
template <typename T>
struct formatter_has_function:
    decltype(formatter_has_function_impl::check<T>(nullptr)) {};


// check type has print member function or not.

struct formatter_has_member_impl {
    template <typename T>
    static auto check(const T* o) -> decltype(
	o->print(std::cout, std::string{}),
	std::true_type{});

    template <typename T>
    static auto check(...) -> std::false_type;
};
template <typename T>
struct formatter_has_member:
    decltype(formatter_has_member_impl::check<T>(nullptr)) {};


// check type has print_as member function or not.

struct formatter_has_printas_impl {
    template <typename T>
    static auto check(const T* o) -> decltype(
	o->print_as(),
	std::true_type{});

    template <typename T>
    static auto check(...) -> std::false_type;
};
template <typename T>
struct formatter_has_printas:
    decltype(formatter_has_printas_impl::check<T>(nullptr)) {};


// check type invoke operator<< function or not.

struct formatter_has_operator_impl {
    template <typename T>
    static auto check(const T* o) -> decltype(
	std::cout << *o,
	std::true_type{});

    template <typename T>
    static auto check(...) -> std::false_type;
};
template <typename T>
struct formatter_has_operator:
    decltype(formatter_has_operator_impl::check<T>(nullptr)) {};


// tag dispatch

struct formatter_int_tag {};
struct formatter_int8_tag {};
struct formatter_uint8_tag {};
struct formatter_float_tag {};
struct formatter_pointer_tag {};
struct formatter_traits_tag {};
struct formatter_function_tag {};
struct formatter_member_tag {};
struct formatter_printas_tag {};
struct formatter_operator_tag {};
struct formatter_enum_tag {};
struct formatter_error_tag {};

template <typename T>
struct formatter_tag {
    typedef typename
    c7::typefunc::ifelse_t<
	std::is_integral<T>,		formatter_int_tag,
	std::is_floating_point<T>,	formatter_float_tag,
	formatter_has_traits<T>,	formatter_traits_tag,
	formatter_has_function<T>,	formatter_function_tag,
	formatter_has_member<T>,	formatter_member_tag,
	formatter_has_printas<T>,	formatter_printas_tag,
	std::is_enum<T>,		formatter_enum_tag,
	formatter_has_operator<T>,	formatter_operator_tag,
	std::true_type,			formatter_error_tag
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
    typedef formatter_operator_tag type;
    static constexpr type value{};
};

template <>
struct formatter_tag<const char*> {
    typedef formatter_operator_tag type;
    static constexpr type value{};
};

template <>
struct formatter_tag<char*> {
    typedef formatter_operator_tag type;
    static constexpr type value{};
};

template <>
struct formatter_tag<int8_t> {
    typedef formatter_int8_tag type;
    static constexpr type value{};
};

template <>
struct formatter_tag<uint8_t> {
    typedef formatter_uint8_tag type;
    static constexpr type value{};
};


/*----------------------------------------------------------------------------
                              special formatter
----------------------------------------------------------------------------*/

template <>
struct format_traits<ssize_t> {
    static c7::delegate<bool, std::ostream&, const std::string&, ssize_t> converter;
    static void convert(std::ostream& out, const std::string& format, ssize_t arg);
};

void print_type(std::ostream& out, const std::string& format, com_status arg);


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
