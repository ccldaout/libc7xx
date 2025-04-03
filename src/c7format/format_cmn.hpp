/*
 * format_cmn.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_FORMAT_CMN_HPP_LOADED_
#define C7_FORMAT_CMN_HPP_LOADED_
#include <c7common.hpp>


#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <c7delegate.hpp>
#include <c7typefunc.hpp>


#if !defined(C7_FORMAT_LIMVEC_N)
# define C7_FORMAT_LIMVEC_N	(64)
#endif


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

struct formatter_charseq_tag {};
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
	c7::typefunc::is_sequence_of<T, char>,	formatter_charseq_tag,
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

template <size_t N>
struct formatter_tag<char[N]> {
    typedef formatter_operator_tag type;
    static constexpr type value{};
};

template <size_t N>
struct formatter_tag<const char[N]> {
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
                                  formatter
----------------------------------------------------------------------------*/

struct format_item {
    enum class format_type {
	arg, arg_ext, width, prec, prefonly,
    };

    format_type type;
    std::ios_base::fmtflags flags;
    const char *pref_beg;
    const char *ext_beg;
    int pref_len;
    int ext_len;
    int width;			// -1: setout outside
    int prec;			// -1: setout outside
    char padding;
    char single_fmt;		// !=0: single basic format is specified

    void set_adjust(std::ios_base::fmtflags adj) {
	flags &= ~(std::ios_base::left|std::ios_base::internal|std::ios_base::right);
	flags |= adj;
    }
};


template <typename T, size_t N = C7_FORMAT_LIMVEC_N>
class limited_vector {
public:
    constexpr limited_vector(): n_(0) {}

    constexpr void push_back(const T& item) {
	if (n_ < N) {
	    vec_[n_] = item;
	    n_++;
	}
    }

    constexpr T& operator[](int index) {
	return vec_[index];
    }

    constexpr const T& operator[](int index) const {
	return vec_[index];
    }

    constexpr size_t size() const {
	return n_;
    }

    constexpr size_t capacity() const {
	return N;
    }

private:
    size_t n_;
    T vec_[N];
};


template <typename FormatItemVec>
void analyze_format(const char *s, FormatItemVec& fmtv);


class formatter {
private:
    std::ostream& out_;

    template <typename T>
    void handle_arg(const format_item& fmt, const T& arg, formatter_charseq_tag) noexcept {
	for (auto c: arg) {
	    out_ << c;
	}
    }

    template <typename T>
    inline void handle_arg(const format_item& fmt, const T& arg, formatter_enum_tag) noexcept {
	handle_arg<ssize_t>(fmt, static_cast<ssize_t>(arg), formatter_int_tag());
    }

    inline std::string ext(const format_item& fmt) noexcept {
	return std::string(fmt.ext_beg, fmt.ext_len);
    }

    template <typename T>
    void handle_inttype_ext(const format_item& fmt, T arg) noexcept;

    template <typename T>
    void handle_arg(const format_item& fmt, T arg, formatter_int_tag) noexcept {
	if (fmt.type == format_item::format_type::arg) {
	    out_ << arg;
	} else {
	    handle_inttype_ext(fmt, arg);
	}
    }

    template <typename T>
    void handle_arg(const format_item& fmt, T arg, formatter_int8_tag) noexcept {
	if (fmt.type == format_item::format_type::arg) {
	    if (fmt.single_fmt) {
		out_ << static_cast<int>(arg);
	    } else {
		out_ << arg;
	    }
	} else {
	    handle_inttype_ext(fmt, static_cast<int>(arg));
	}
    }

    template <typename T>
    void handle_arg(const format_item& fmt, T arg, formatter_uint8_tag) noexcept {
	if (fmt.type == format_item::format_type::arg) {
	    if (fmt.single_fmt) {
		out_ << static_cast<unsigned int>(arg);
	    } else {
		out_ << arg;
	    }
	} else {
	    handle_inttype_ext(fmt, static_cast<unsigned int>(arg));
	}
    }

    template <typename T>
    void handle_arg(const format_item& fmt, T arg, formatter_float_tag) noexcept {
	out_ << arg;
    }

    template <typename T>
    inline void handle_arg(const format_item& fmt, T arg, formatter_pointer_tag) noexcept {
	out_ << arg;
    }

    template <typename T>
    void handle_arg(const format_item& fmt, const T& arg, formatter_traits_tag) noexcept {
	format_traits<T>::convert(out_, ext(fmt), arg);
    }

    template <typename T>
    void handle_arg(const format_item& fmt, const T& arg, formatter_function_tag) noexcept {
	print_type(out_, ext(fmt), arg);
    }

    template <typename T>
    void handle_arg(const format_item& fmt, const T& arg, formatter_member_tag) noexcept {
	arg.print(out_, ext(fmt));
    }

    template <typename T>
    void handle_arg(const format_item& fmt, const T& arg, formatter_printas_tag) noexcept {
	const auto& as_value = arg.print_as();
	using U = std::remove_reference_t<std::remove_cv_t<decltype(as_value)>>;
	handle_arg<U>(fmt, as_value, formatter_tag<U>::value);
    }

    template <typename T>
    inline void handle_arg(const format_item& fmt, const T& arg, formatter_operator_tag) noexcept {
	out_ << arg;
    }

    template <typename T>
    inline void handle_arg(const format_item& fmt, const T& arg, formatter_error_tag) noexcept {
	static_assert(c7::typefunc::false_v<T>, "Specified type is not formattable.");
    }

    const format_item& apply_item(const format_item * const fmts, size_t nfmt, size_t& index) noexcept;

    void apply_item_last(const format_item * const fmts, size_t nfmt, size_t& index) noexcept;

public:
    explicit formatter(std::ostream& out): out_(out) {}

    template <typename AnalyzedFormat>
    void apply(const AnalyzedFormat& fmts, size_t index) noexcept {
	apply_item_last(&fmts[0], fmts.size(), index);
    }

    template <typename AnalyzedFormat, typename Arg, typename... Args>
    void apply(const AnalyzedFormat& fmts, size_t index, Arg& arg, Args&... args) noexcept {
	using U = std::remove_reference_t<std::remove_cv_t<Arg>>;
	auto& fmt = apply_item(&fmts[0], fmts.size(), index);
	handle_arg<U>(fmt, arg, formatter_tag<U>::value);
	apply(fmts, index+1, args...);
    }
};


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
