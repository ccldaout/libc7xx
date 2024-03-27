/*
 * format_r2.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_FORMAT_R2_HPP_LOADED_
#define C7_FORMAT_R2_HPP_LOADED_
#include <c7common.hpp>


#include <c7format/format_cmn.hpp>
#include <c7strmbuf/strref.hpp>


#define C7_FORMAT_ANALYZED_FORMAT (1)


#if !defined(C7_FORMAT_LIMVEC_N)
# define C7_FORMAT_LIMVEC_N	(64)
#endif


namespace c7::format_r2 {


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


template <typename FormatItemVec>
void analyze_format(const char *s, FormatItemVec& fmtv);


template <typename T>
class limited_vector {
public:
    static constexpr size_t N = C7_FORMAT_LIMVEC_N;

    limited_vector(): n_(0) {}

    void push_back(const T& item) {
	if (n_ < N) {
	    vec_[n_] = item;
	    n_++;
	}
    }

    T& operator[](int index) {
	return vec_[index];
    }

    const T& operator[](int index) const {
	return vec_[index];
    }

    size_t size() const {
	return n_;
    }

private:
    size_t n_;
    T vec_[N];
};


class analyzed_format {
public:
    explicit analyzed_format(const char *s) {
	analyze_format(s, fmts_);
    }

    explicit analyzed_format(const std::string& s): analyzed_format(s.c_str()) {}

    size_t size() {
	return fmts_.size();
    }

    size_t size() const {
	return fmts_.size();
    }

    format_item& operator[](int index) {
	return fmts_[index];
    }

    const format_item& operator[](int index) const {
	return fmts_[index];
    }

private:
    limited_vector<format_item> fmts_;
};


class formatter {
private:
    std::ostream& out_;

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
                                 entry point
----------------------------------------------------------------------------*/

// add to stream

template <typename... Args>
inline void format(std::ostream& out, const analyzed_format& fmts, const Args&... args) noexcept
{
    formatter formatter(out);
    formatter.apply(fmts, 0, args...);
}

template <typename... Args>
inline void format(std::ostream& out, const char *fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    format(out, fmts, args...);
}

template <typename... Args>
inline void format(std::ostream& out, const std::string& fmt, const Args&... args) noexcept
{
    format(out, fmt.c_str(), args...);
}

// add to string

template <typename... Args>
inline void format(std::string& str, const analyzed_format& fmts, const Args&... args) noexcept
{
    auto sb = c7::strmbuf::strref(str);
    auto os = std::basic_ostream(&sb);
    formatter formatter(os);
    formatter.apply(fmts, 0, args...);
}

template <typename... Args>
inline void format(std::string& str, const char *fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    format(str, fmts, args...);
}

template <typename... Args>
inline void format(std::string& str, const std::string& fmt, const Args&... args) noexcept
{
    format(str, fmt.c_str(), args...);
}

// new string

template <typename... Args>
inline std::string format(const analyzed_format& fmts, const Args&... args) noexcept
{
    std::string str;
    format(str, fmts, args...);
    return str;
}

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
inline void P_(const analyzed_format& fmts, const Args&... args) noexcept
{
    c7::format_cmn::cout_lock lock;
    formatter formatter(std::cout);
    formatter.apply(fmts, 0, args...);
}

template <typename... Args>
inline void P_(const char *fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    P_(fmts, args...);
}

template <typename... Args>
inline void P_(const std::string& fmt, const Args&... args) noexcept
{
    P_(fmt.c_str(), args...);
}

// std::cout without NL(new line) and without lock

template <typename... Args>
inline void P_nolock(const analyzed_format& fmts, const Args&... args) noexcept
{
    formatter formatter(std::cout);
    formatter.apply(fmts, 0, args...);
}

template <typename... Args>
inline void P_nolock(const char *fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    P_nolock(fmts, args...);
}

template <typename... Args>
inline void P_nolock(const std::string& fmt, const Args&... args) noexcept
{
    P_nolock(fmt.c_str(), args...);
}

// std::cout with NL(new line) and with lock

template <typename... Args>
inline void p_(const analyzed_format& fmts, const Args&... args) noexcept
{
    c7::format_cmn::cout_lock lock;
    formatter formatter(std::cout);
    formatter.apply(fmts, 0, args...);
    std::cout << std::endl;
}

template <typename... Args>
inline void p_(const char *fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    p_(fmts, args...);
}

template <typename... Args>
inline void p_(const std::string& fmt, const Args&... args) noexcept
{
    p_(fmt.c_str(), args...);
}

// std::cout with NL(new line) and without lock

template <typename... Args>
inline void p_nolock(const analyzed_format& fmts, const Args&... args) noexcept
{
    formatter formatter(std::cout);
    formatter.apply(fmts, 0, args...);
    std::cout << std::endl;
}

template <typename... Args>
inline void p_nolock(const char *fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    p_nolock(fmts, args...);
}

template <typename... Args>
inline void p_nolock(const std::string& fmt, const Args&... args) noexcept
{
    p_nolock(fmt.c_str(), args...);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::format_r2


#endif // format_r2.hpp
