/*
 * c7string/c_str.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=336895331
 */
#ifndef C7_STRING_C_STR_HPP_LOADED_
#define C7_STRING_C_STR_HPP_LOADED_
#include <c7common.hpp>


#include <cctype>
#include <cstdlib>
#include <cstring>
#include <c7nseq/enumerate.hpp>


namespace c7 {


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
    return strmatch_impl(s, index, c.c_str(), cands...);
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
    return strmatch_head_impl(s, index, c.c_str(), cands...);
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
    return strmatch_tail_impl(e, en, index, c.c_str(), cands...);
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


} // namespace c7


#endif // c7string/c_str.hpp
