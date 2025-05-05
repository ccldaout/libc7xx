/*
 * c7string/basic.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7string/basic.hpp>
#include <c7string/eval.hpp>


namespace c7::str {


std::string trim(const std::string& in, const std::string& removed_chars)
{
    const char *beg = in.c_str();
    const char *end = beg + in.size();
    const char *rmv = removed_chars.c_str();
    beg = strskip(beg, rmv);
    while (beg < end) {
	if (std::strchr(rmv, *--end) == nullptr) {
	    end++;
	    break;
	}
    }
    return std::string(beg, end - beg);
}


std::string trim_left(const std::string& in, const std::string& removed_chars)
{
    const char *beg = in.c_str();
    const char *end = beg + in.size();
    const char *rmv = removed_chars.c_str();
    beg = strskip(beg, rmv);
    return std::string(beg, end - beg);
}


std::string trim_right(const std::string& in, const std::string& removed_chars)
{
    const char *beg = in.c_str();
    const char *end = beg + in.size();
    const char *rmv = removed_chars.c_str();
    while (beg < end) {
	if (std::strchr(rmv, *--end) == nullptr) {
	    end++;
	    break;
	}
    }
    return std::string(beg, end - beg);
}


std::string replace(const std::string& s, const std::string& o, const std::string& n)
{
    std::string out;
    auto p= s.c_str();
    auto op = o.c_str();
    for (;;) {
        auto q = std::strstr(p, op);
        if (q == nullptr) {
            out += p;
            return out;
        }
        out.append(p, q);
        out += n;
        p = q + o.size();
    }
}


} // namespace c7::str


namespace std {

/*----------------------------------------------------------------------------
                   print_type for std::vector<std::string>
----------------------------------------------------------------------------*/

void print_type(std::ostream& o, const std::string& fmt, const vector<string>& sv)
{
    if (fmt.empty()) {
	o << "{ ";
	for (auto& s: sv) {
	    o << '"';
	    c7::str::escape_C(o, s, '"');
	    o << "\" ";
	}
	o << "}";
    } else {
	c7::str::concat(o, fmt, sv);
    }
}

} // namespcae std
