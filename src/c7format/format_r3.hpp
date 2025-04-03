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
#include <c7strmbuf/strref.hpp>


#define C7_FORMAT_ANALYZED_FORMAT 	(1)
#define C7_FORMAT_LITERAL		(1)


namespace c7::format_r3 {


using c7::format_cmn::format_item;
using c7::format_cmn::analyze_format;
using c7::format_cmn::formatter;


/*----------------------------------------------------------------------------
                                  formatter
----------------------------------------------------------------------------*/

class analyzed_format {
public:
    analyzed_format() = default;

    explicit analyzed_format(const char *s) {
	init(s);
    }

    explicit analyzed_format(const std::string& s) {
	init(s.data());
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

    static const analyzed_format& get_for_literal(const char *fmt) {
	const auto& [it, _] = p_dic_.try_emplace(fmt, fmt);
	return (*it).second;
    }

private:
    std::vector<format_item> fmts_;

    static thread_local std::unordered_map<const void*, analyzed_format> p_dic_;

    void init(const char *s) {
	analyze_format(s, fmts_);
    }
};


// C7_FORMAT_LITERAL
inline const analyzed_format& operator""_F(const char *s, size_t)
{
    return analyzed_format::get_for_literal(s);
}


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

template <size_t N, typename... Args>
inline void format(std::ostream& out, const char (&fmt)[N], const Args&... args) noexcept
{
    format(out, analyzed_format::get_for_literal(&fmt[0]), args...);
}

template <typename... Args>
inline void format(std::ostream& out, const std::string& fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    format(out, fmts, args...);
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

template <size_t N, typename... Args>
inline void format(std::string& str, const char (&fmt)[N], const Args&... args) noexcept
{
    format(str, analyzed_format::get_for_literal(&fmt[0]), args...);
}

template <typename... Args>
inline void format(std::string& str, const std::string& fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    format(str, fmts, args...);
}

// new string

template <typename... Args>
inline std::string format(const analyzed_format& fmts, const Args&... args) noexcept
{
    std::string str;
    format(str, fmts, args...);
    return str;
}

template <size_t N, typename... Args>
inline std::string format(const char (&fmt)[N], const Args&... args) noexcept
{
    std::string str;
    format(str, analyzed_format::get_for_literal(&fmt[0]), args...);
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

template <size_t N, typename... Args>
inline void P_(const char (&fmt)[N], const Args&... args) noexcept
{
    P_(analyzed_format::get_for_literal(&fmt[0]), args...);
}

template <typename... Args>
inline void P_(const std::string& fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    P_(fmts, args...);
}

// std::cout without NL(new line) and with lock

template <typename... Args>
inline void P_nolock(const analyzed_format& fmts, const Args&... args) noexcept
{
    formatter formatter(std::cout);
    formatter.apply(fmts, 0, args...);
}

template <size_t N, typename... Args>
inline void P_nolock(const char (&fmt)[N], const Args&... args) noexcept
{
    P_nolock(analyzed_format::get_for_literal(&fmt[0]), args...);
}

template <typename... Args>
inline void P_nolock(const std::string& fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    P_nolock(fmts, args...);
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

template <size_t N, typename... Args>
inline void p_(const char (&fmt)[N], const Args&... args) noexcept
{
    p_(analyzed_format::get_for_literal(&fmt[0]), args...);
}

template <typename... Args>
inline void p_(const std::string& fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    p_(fmts, args...);
}

// std::cout with NL(new line) and without lock

template <typename... Args>
inline void p_nolock(const analyzed_format& fmts, const Args&... args) noexcept
{
    formatter formatter(std::cout);
    formatter.apply(fmts, 0, args...);
    std::cout << std::endl;
}

template <size_t N, typename... Args>
inline void p_nolock(const char (&fmt)[N], const Args&... args) noexcept
{
    p_nolock(analyzed_format::get_for_literal(&fmt[0]), args...);
}

template <typename... Args>
inline void p_nolock(const std::string& fmt, const Args&... args) noexcept
{
    analyzed_format fmts{fmt};
    p_nolock(fmts, args...);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::format_r3


#endif // format_r3.hpp
