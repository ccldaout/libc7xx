/*
 * format_r3.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_FORMAT_R3_HPP_LOADED_
#define C7_FORMAT_R3_HPP_LOADED_
#include <c7common.hpp>


#include <c7format/format_cmn.hpp>
#include <c7format/format_r2.hpp>
#include <c7strmbuf/strref.hpp>


#define C7_FORMAT_ANALYZED_FORMAT (1)


namespace c7::format_r3 {


/*----------------------------------------------------------------------------
                                  formatter
----------------------------------------------------------------------------*/

using c7::format_r2::format_item;
using c7::format_r2::formatter;


class analyzed_format {
public:
    analyzed_format() = default;

    explicit analyzed_format(const char *s) {
	init(s);
    }

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

    static const analyzed_format& get(const char *fmt) {
	const auto& [it, inserted] = s_dic_.try_emplace(fmt);
	if (inserted) {
	    (*it).second.init(fmt);
	}
	return (*it).second;
    }

    static const analyzed_format& get_for_literal(const char *fmt) {
	const auto& [it, inserted] = v_dic_.try_emplace(fmt);
	if (inserted) {
	    (*it).second.init(fmt);
	}
	return (*it).second;
    }

private:
    std::vector<format_item> fmts_;

    static thread_local std::unordered_map<std::string, analyzed_format> s_dic_;
    static thread_local std::unordered_map<const void*, analyzed_format> v_dic_;

    void init(const char *s) {
	c7::format_r2::analyze_format(s, fmts_);
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
    format(out, analyzed_format::get_for_literal(fmt), args...);
}

template <typename... Args>
inline void format_var(std::ostream& out, const char *fmt, const Args&... args) noexcept
{
    format(out, analyzed_format::get(fmt), args...);
}

template <typename... Args>
inline void format(std::ostream& out, const std::string& fmt, const Args&... args) noexcept
{
    format_var(out, fmt.c_str(), args...);
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
    format(str, analyzed_format::get_for_literal(fmt), args...);
}

template <typename... Args>
inline void format_var(std::string& str, const char *fmt, const Args&... args) noexcept
{
    format(str, analyzed_format::get(fmt), args...);
}

template <typename... Args>
inline void format(std::string& str, const std::string& fmt, const Args&... args) noexcept
{
    format_var(str, fmt.c_str(), args...);
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
inline std::string format_var(const char *fmt, const Args&... args) noexcept
{
    std::string str;
    format_var(str, fmt, args...);
    return str;
}

template <typename... Args>
inline std::string format(const std::string& fmt, const Args&... args) noexcept
{
    std::string str;
    format(str, fmt, args...);
    return str;
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
    P_(analyzed_format::get_for_literal(fmt), args...);
}

template <typename... Args>
inline void P_var(const char *fmt, const Args&... args) noexcept
{
    P_(analyzed_format::get(fmt), args...);
}

template <typename... Args>
inline void P_(const std::string& fmt, const Args&... args) noexcept
{
    P_(analyzed_format::get(fmt.c_str()), args...);
}

// std::cout without NL(new line) and with lock

template <typename... Args>
inline void P_nolock(const analyzed_format& fmts, const Args&... args) noexcept
{
    formatter formatter(std::cout);
    formatter.apply(fmts, 0, args...);
}

template <typename... Args>
inline void P_nolock(const char *fmt, const Args&... args) noexcept
{
    P_nolock(analyzed_format::get_for_literal(fmt), args...);
}

template <typename... Args>
inline void P_nolock_var(const char *fmt, const Args&... args) noexcept
{
    P_nolock(analyzed_format::get(fmt), args...);
}

template <typename... Args>
inline void P_nolock(const std::string& fmt, const Args&... args) noexcept
{
    P_nolock(analyzed_format::get(fmt.c_str()), args...);
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
    p_(analyzed_format::get_for_literal(fmt), args...);
}

template <typename... Args>
inline void p_var(const char *fmt, const Args&... args) noexcept
{
    p_(analyzed_format::get(fmt), args...);
}

template <typename... Args>
inline void p_(const std::string& fmt, const Args&... args) noexcept
{
    p_(analyzed_format::get(fmt.c_str()), args...);
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
    p_nolock(analyzed_format::get_for_literal(fmt), args...);
}

template <typename... Args>
inline void p_nolock_var(const char *fmt, const Args&... args) noexcept
{
    p_nolock(analyzed_format::get(fmt), args...);
}

template <typename... Args>
inline void p_nolock(const std::string& fmt, const Args&... args) noexcept
{
    p_nolock(analyzed_format::get(fmt.c_str()), args...);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::format_r3


#endif // format_r3.hpp
