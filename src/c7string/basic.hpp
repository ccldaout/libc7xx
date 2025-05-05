/*
 * c7string/basic.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=336895331
 */
#ifndef C7_STRING_BASIC_HPP_LOADED_
#define C7_STRING_BASIC_HPP_LOADED_
#include <c7common.hpp>


#include <cstring>		// strchr
#include <ostream>
#include <string>
#include <vector>
#include <c7coroutine.hpp>	// generator
#include <c7nseq/head.hpp>
#include <c7result.hpp>


#include <c7string/c_str.hpp>


#define C7_STRING_REPLACE	(1U)


namespace std {

inline std::ostream& operator+=(std::ostream& out, const std::string& in)
{
    out.write(in.c_str(), in.size());
    return out;
}

inline std::ostream& operator+=(std::ostream& out, const char in)
{
    out.put(in);
    return out;
}

void print_type(std::ostream& o, const std::string& fmt, const vector<string>& sv);

} // namespace std


namespace c7::str {


// upper

template <typename S>
inline S& upper(S& out, const std::string& s, int n)
{
    for (auto p = s.c_str(); n > 0; n--, p++) {
	out += std::toupper(static_cast<int>(*p));
    }
    return out;
}

template <typename S>
inline S& upper(S& out, const std::string& s, const char *e)
{
    return upper(out, s, e - s.c_str());
}

template <typename S>
inline S& upper(S& out, const std::string& s)
{
    return upper(out, s, s.size());
}


// lower

template <typename S>
inline S& lower(S& out, const std::string& s, size_t n)
{
    for (auto p = s.c_str(); n > 0; n--, p++) {
	out += std::tolower(static_cast<int>(*p));
    }
    return out;
}

template <typename S>
inline S& lower(S& out, const std::string& s, const char *e)
{
    return lower(out, s, e - s.c_str());
}

template <typename S>
inline S& lower(S& out, const std::string& s)
{
    return lower(out, s, s.size());
}


// concat

template <typename S>
S& concat(S& out, const std::string& sep, const char **sv, int sc = -1)
{
    if (*sv == nullptr) {
	return out;
    }
    out += *sv++;
    if (sc == -1) {
	for (; *sv != nullptr; sv++) {
	    (out += sep) += *sv;
	}
    } else {
	for (sc--; sc > 0; sc--, sv++) {
	    (out += sep) += *sv;
	}
    }
    return out;
}

template <typename S, typename S2>
S& concat(S& out, const std::string& sep, const std::vector<S2>& sv)
{
    if (sv.empty()) {
	return out;
    }
    out += sv[0];
    for (const auto& s: sv | c7::nseq::skip_head(1)) {
	(out += sep) += s;
    }
    return out;
}

inline std::string join(const std::string& sep, const char **sv, int sc = -1)
{
    std::string out;
    return concat(out, sep, sv, sc);
}

template <typename S2>
std::string join(const std::string& sep, const std::vector<S2>& sv)
{
    std::string out;
    return concat(out, sep, sv);
}


// trim

std::string trim(const std::string& in, const std::string& removed_chars);

inline std::string trim(const std::string& in)
{
    return trim(in, " \t\n\r");
}

std::string trim_left(const std::string& in, const std::string& removed_chars);

inline std::string trim_left(const std::string& in)
{
    return trim_left(in, " \t\n\r");
}

std::string trim_right(const std::string& in, const std::string& removed_chars);

inline std::string trim_right(const std::string& in)
{
    return trim_right(in, " \t\n\r");
}


// replace: C7_STRING_REPLACE

std::string replace(const std::string& s, const std::string& o, const std::string& n);


// transpose

template <typename Trans>
inline std::string transpose(const std::string& in, Trans trans)
{
    std::string out;
    for (auto ch: in) {
	if (auto c = trans(ch); c != 0) {
	    out += c;
	}
    }
    return out;
}

inline std::string transpose(const std::string& in, const std::string& cond, const std::string& trans)
{
    auto last_trans = trans[trans.size()-1];
    return transpose(in, [&cond, &trans, last_trans](char ch) {
	    if (auto pos = cond.find(ch); pos != std::string::npos) {
		return (pos < trans.size()) ? trans[pos] : last_trans;
	    }
	    return ch;
	});
}


// split

template <typename Func,
	  std::enable_if_t<std::is_invocable_v<Func, std::string>>* = nullptr>
inline void split_for(const std::string& in, char sep, Func func)
{
    const char *beg = in.c_str();
    const char *p;
    while ((p = std::strchr(beg, sep)) != nullptr) {
	func(std::string(beg, p-beg));
	beg = p + 1;
    }
    func(std::string(beg));
}

template <typename Func,
	  std::enable_if_t<!std::is_invocable_v<Func, std::string>>* = nullptr>
inline void split_for(const std::string& in, char sep, Func func)
{
    const char *beg = in.c_str();
    const char *p;
    while ((p = std::strchr(beg, sep)) != nullptr) {
	func(beg, p);
	beg = p + 1;
    }
    func(beg, std::strchr(beg, 0));
}

inline c7::generator<std::string>
split_gen(const std::string& in, char sep)
{
    auto lambda = [in,sep]() {
	split_for(in, sep, [](std::string&& s) {
		c7::generator<std::string>::yield(s);
	    });
    };
    return c7::generator<std::string>(1024, lambda);
}

template <typename Conv>
inline std::vector<std::string>
split(const std::string& in, char sep, Conv conv)
{
    std::vector<std::string> sv;
    split_for(in, sep, [&sv, &conv](const char *beg, const char *end) {
	    sv.push_back(conv(std::string(beg, end - beg)));
	});
    return sv;
}

inline std::vector<std::string>
split(const std::string& in, char sep)
{
    return split(in, sep, [](const std::string& s){return s;});
}

inline std::vector<std::string>
split_trim(const std::string& in, char sep, const std::string& removed_chars)
{
    return split(in, sep, [&removed_chars](const std::string& s) {
	    return trim(s, removed_chars);
	});
}

inline std::vector<std::string>
split_trim(const std::string& in, char sep)
{
    return split_trim(in, sep, " \t\n\r");
}


} // namespace c7::str


#endif // c7string.hpp
