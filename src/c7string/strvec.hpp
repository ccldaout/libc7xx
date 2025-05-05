/*
 * c7string/strvec.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=336895331
 */
#ifndef C7_STRING_STRVEC_HPP_LOADED_
#define C7_STRING_STRVEC_HPP_LOADED_
#include <c7common.hpp>


#include <ostream>
#include <string>
#include <vector>


// specialized std::vector<std::string>


#define C7_STRVEC_CHARPP	(1U)


namespace c7 {


class strvec: public std::vector<std::string> {
private:
    typedef std::vector<std::string> base_t;

public:
    using base_t::vector;

    // copy, move constructors and assignments are all defined as default

    strvec(const std::vector<std::string>& sv): base_t(sv) {}

    strvec(std::vector<std::string>&& sv): base_t(std::move(sv)) {}

    // C7_STRVEC_CHARPP
    explicit strvec(const char **pv) {
	*this += pv;
    }

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


} // namespace c7


namespace std {

#if !defined(C7_STRING_DISABLE_PRINT_TYPE)
void print_type(std::ostream& o, const std::string& fmt, const vector<string>& sv);
#endif

} // namespace std


#endif // c7string/strvec.hpp
