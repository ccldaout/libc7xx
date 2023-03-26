/*
 * format_cmn.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_FORMAT_CMN_HPP_LOADED__
#define C7_FORMAT_CMN_HPP_LOADED__
#include <c7common.hpp>


#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <c7delegate.hpp>
#include <c7typefunc.hpp>


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
----------------------------------------------------------------------------*/


} // namespace c7


namespace c7::format_cmn {


/*----------------------------------------------------------------------------
                              lock for std::cout
----------------------------------------------------------------------------*/

class cout_lock {
private:
    static uint64_t lock_;

public:
    cout_lock() {
	while (__sync_lock_test_and_set(&lock_, 1) == 1);
    }
    ~cout_lock() {
	__sync_lock_release(&lock_);
    }
};


/*----------------------------------------------------------------------------
                            format control helper
----------------------------------------------------------------------------*/

#define C7_CONFIG_FORMAT_CACHE	(1)

class format_cache {
private:
    std::ostream& out_;
    std::ios_base::fmtflags flags_;
    std::streamsize prec_;
    std::streamsize width_;
    char padding_;

public:
    explicit format_cache(std::ostream& out):
	out_(out),
	flags_(out.flags()),
	prec_(out.precision(0)),
	width_(out.width(0)),
	padding_(out.fill()) {
    }

    template <typename T>
    void with_default(const T& v) {
	out_.flags(std::ios_base::fmtflags());
	out_.precision(0);
	out_.width(0);
	out_.fill(' ');
	out_ << v;
    }

    template <typename T>
    void with_restore(const T& v) {
	out_.flags(flags_);
	out_.precision(prec_);
	out_.width(width_);
	out_.fill(padding_);
	out_ << v;
    }
};


} // namespace c7::format_cmn


#endif // format_cmn.hpp
