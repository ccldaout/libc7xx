/*
 * c7string.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=336895331
 */
#ifndef C7_STRING_HPP_LOADED__
#define C7_STRING_HPP_LOADED__
#include <c7common.hpp>


#include <c7coroutine.hpp>
#include <c7nseq/enumerate.hpp>
#include <c7nseq/head.hpp>
#include <c7result.hpp>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>


namespace c7 {


/*----------------------------------------------------------------------------
                                   C string
----------------------------------------------------------------------------*/

inline const char *getenv_x(const char *env, const char *alt = nullptr)
{
    const char *p = std::getenv(env);
    return (p != nullptr) ? p : alt;
}

inline int strcount(const char *s, int ch)
{
    int n = 0;
    while (*s != 0) {
	if (*s++ == ch) {
	    n++;
	}
    }
    return n;
}

inline const char *strskip(const char *s, const char *list)
{
    return s + std::strspn(s, list);
}

inline const char *strskip_ws(const char *s)
{
    return strskip(s, " \t");
}

inline const char *strfind(const char *s, const char *list)
{
    return s + std::strcspn(s, list);
}

inline const char *strfind_ws(const char *s)
{
    return strfind(s, " \t");
}

inline const char *strskip_on(const char *s, const unsigned int *tab, unsigned int mask)
{
    for (; *s != 0 && (tab[(unsigned)*s] & mask) != 0; s++) {
        ;
    }
    return s;
}

inline const char *strfind_on(const char *s, const unsigned int *tab, unsigned int mask)
{
    for (; *s != 0 && (tab[(unsigned)*s] & mask) == 0; s++) {
        ;
    }
    return s;
}

inline const char *strchr_x(const char *s, char c, const char *alt)
{
    const char *p = std::strchr(s, c);
    return (p != 0) ? p : ((alt == 0) ? std::strchr(s, 0) : (char *)alt);
}

inline const char *strchr_next(const char *s, char c, const char *alt)
{
    const char *p = std::strchr(s, c);
    return (p != 0) ? (p + 1) : ((alt == 0) ? std::strchr(s, 0) : (char *)alt);
}

inline const char *strrchr_x(const char *s, char c, const char *alt)
{
    const char *p = std::strrchr(s, c);
    return (p != 0) ? p : ((alt == 0) ? std::strchr(s, 0) : (char *)alt);
}

inline const char *strrchr_next(const char *s, char c, const char *alt)
{
    const char *p = std::strrchr(s, c);
    return (p != 0) ? (p + 1) : ((alt == 0) ? std::strchr(s, 0) : (char *)alt);
}

inline const char *strpbrk_x(const char *s, const char *c, const char *alt)
{
    const char *p = std::strpbrk(s, c);
    return (p != 0) ? p : ((alt == 0) ? std::strchr(s, 0) : (char *)alt);
}

inline const char *strpbrk_next(const char *s, const char *c, const char *alt)
{
    const char *p = std::strpbrk(s, c);
    return (p != 0) ? (p + 1) : ((alt == 0) ? std::strchr(s, 0) : (char *)alt);
}

inline char *strcpy(char *s, const char *b)
{
    return std::strchr(std::strcpy(s, b), 0);
}

inline char *strbcpy(char *s, const char *b, const char *e)
{
    auto n = e - b;
    (void)std::memcpy(s, b, n);
    s[n] = 0;
    return &s[n];
}

// strmatch

inline int strmatch_impl(const char *, int)
{
    return -1;
}

template <typename... Cands>
inline int strmatch_impl(const char *s, int index, const char *c, Cands... cands)
{
    if (std::strcmp(s, c) == 0) {
	return index;
    }
    return strmatch_impl(s, index+1, cands...);
}

template <typename... Cands>
inline int strmatch_impl(const char *s, int index, const std::string& c, Cands... cands)
{
    return strmatch_impl(s, index+1, c.c_str(), cands...);
}

template <typename... Cands>
inline int strmatch(const std::string& s, Cands... cands)
{
    return strmatch_impl(s.c_str(), 0, cands...);
}

inline int strmatch(const std::string& s, const char *cand_v[], int cand_n = -1)
{
    if (cand_n == -1) {
	for (cand_n = 0; cand_v[cand_n] != nullptr; cand_n++);
    }
    for (int i = 0; i < cand_n; i++) {
	if (std::strcmp(s.c_str(), cand_v[i]) == 0) {
	    return i;
	}
    }
    return -1;
}

template <typename S>
inline int strmatch(const std::string& s, const std::vector<S>& cands)
{
    for (const auto& [i, c]: cands | c7::nseq::enumerate()) {
	if (s == c) {
	    return i;
	}
    }
    return -1;
}

// strmatch_head

inline int strmatch_head_impl(const char *, int)
{
    return -1;
}

template <typename... Cands>
inline int strmatch_head_impl(const char *s, int index, const char *c, Cands... cands)
{
    if (std::strncmp(s, c, std::strlen(c)) == 0) {
	return index;
    }
    return strmatch_head_impl(s, index+1, cands...);
}

template <typename... Cands>
inline int strmatch_head_impl(const char *s, int index, const std::string& c, Cands... cands)
{
    return strmatch_head_impl(s, index+1, c.c_str(), cands...);
}

template <typename... Cands>
inline int strmatch_head(const std::string& s, Cands... cands)
{
    return strmatch_head_impl(s.c_str(), 0, cands...);
}

inline int strmatch_head(const std::string& s, const char *cand_v[], int cand_n = -1)
{
    if (cand_n == -1) {
	for (cand_n = 0; cand_v[cand_n] != nullptr; cand_n++);
    }
    for (int i = 0; i < cand_n; i++) {
	if (std::strncmp(s.c_str(), cand_v[i], std::strlen(cand_v[i])) == 0) {
	    return i;
	}
    }
    return -1;
}

template <typename S>
inline int strmatch_head(const std::string& s, const std::vector<S>& cands)
{
    for (const auto& [i, c]: cands | c7::nseq::enumerate()) {
	if (s.compare(0, c.size(), c) == 0) {
	    return i;
	}
    }
    return -1;
}

// strmatch_tail

inline int strmatch_tail_impl(const char *, int, int)
{
    return -1;
}

template <typename... Cands>
inline int strmatch_tail_impl(const char *e, int en, int index, const char *c, Cands... cands)
{
    int cn = std::strlen(c);
    if (cn <= en && std::strcmp(e-cn, c) == 0) {
	return index;
    }
    return strmatch_tail_impl(e, en, index+1, cands...);
}

template <typename... Cands>
inline int strmatch_tail_impl(const char *e, int en, int index, const std::string& c, Cands... cands)
{
    return strmatch_tail_impl(e, en, index+1, c.c_str(), cands...);
}

template <typename... Cands>
inline int strmatch_tail(const std::string& ss, Cands... cands)
{
    const char *s = ss.c_str();
    const char *e = std::strchr(s, 0);
    return strmatch_tail_impl(e, e-s, 0, cands...);
}

inline int strmatch_tail(const std::string& ss, const char *cand_v[], int cand_n = -1)
{
    if (cand_n == -1) {
	for (cand_n = 0; cand_v[cand_n] != nullptr; cand_n++);
    }

    const char *s = ss.c_str();
    const char *e = std::strchr(s, 0);
    int en = e - s;

    for (int i = 0; i < cand_n; i++, cand_v++) {
	int cn = std::strlen(*cand_v);
	if (cn <= en && std::strcmp(e-cn, *cand_v) == 0) {
	    return i;
	}
    }
    return -1;
}

template <typename S>
inline int strmatch_tail(const std::string& s, const std::vector<S>& cands)
{
    for (auto [i, c]: cands | c7::nseq::enumerate()) {
	if (s.size() >= c.size() && s.compare(s.size() - c.size(), c.size(), c) == 0) {
	    return i;
	}
    }
    return -1;
}


/*----------------------------------------------------------------------------
                     specialized std::vector<std::string>
----------------------------------------------------------------------------*/

class strvec: public std::vector<std::string> {
private:
    typedef std::vector<std::string> base_t;

public:
    using base_t::vector;

    // copy, move constructors and assignments are all defined as default

    strvec(const std::vector<std::string>& sv): base_t(sv) {}

    strvec(std::vector<std::string>&& sv): base_t(std::move(sv)) {}

    strvec& operator=(const std::string& s) {
	clear();
	push_back(s);
	return *this;
    }

    strvec& operator+=(const std::string& s) {
	push_back(s);
	return *this;
    }

    strvec& operator=(std::string&& s) {
	clear();
	push_back(std::move(s));
	return *this;
    }

    strvec& operator+=(std::string&& s) {
	push_back(std::move(s));
	return *this;
    }

    strvec& operator=(const char **pv) {
	clear();
	return (*this += pv);
    }

    strvec& operator+=(const char **pv) {
	for (; *pv != nullptr; pv++) {
	    push_back(*pv);
	}
	return *this;
    }

    strvec& operator+=(const std::vector<std::string>& sv) {
	insert(end(), sv.begin(), sv.end());
	return *this;
    }

    strvec& operator+=(std::vector<std::string>&& sv) {
	reserve(size() + sv.size());
	for (auto&& s: std::move(sv)) {
	    push_back(std::move(s));
	}
	return *this;
    }

    std::vector<char *> c_strv() const {
	std::vector<char *> csv;
	for (auto& s: *this) {
	    csv.push_back(const_cast<char *>(s.c_str()));
	}
	csv.push_back(nullptr);
	return csv;
    }
};


/*============================================================================
                                namespace str
============================================================================*/


namespace str {


/*----------------------------------------------------------------------------
                               basic operations
----------------------------------------------------------------------------*/

inline std::stringstream& operator+=(std::stringstream& out, const std::string& in)
{
    out.write(in.c_str(), in.size());
    return out;
}

inline std::stringstream& operator+=(std::stringstream& out, const char in)
{
    out.put(in);
    return out;
}


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


/*----------------------------------------------------------------------------
                            complicated operation
----------------------------------------------------------------------------*/

// eval C escape sequece

std::string& eval_C(std::string& out, const std::string& s);
std::stringstream& eval_C(std::stringstream& out, const std::string& s);
std::string eval_C(const std::string& s);


// eval

typedef std::function<c7::result<const char*>(std::string& out, const char *vn, bool enclosed)> evalvar;

c7::result<> eval(std::string& out, const std::string& in, char mark, char escape, c7::str::evalvar);
c7::result<> eval(std::stringstream& out, const std::string& in, char mark, char escape, c7::str::evalvar);
c7::result<std::string> eval(const std::string& in, char mark, char escape, c7::str::evalvar evalvar);


// eval C escape sequece

c7::result<> eval_env(std::string& out, const std::string& in);
c7::result<> eval_env(std::stringstream& out, const std::string& in);
c7::result<std::string> eval_env(const std::string& in);


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace str
} // namespace c7


#endif // c7string.hpp
