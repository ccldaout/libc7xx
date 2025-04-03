/*
 * format_r1.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_FORMAT_R1_HPP_LOADED_
#define C7_FORMAT_R1_HPP_LOADED_
#include <c7common.hpp>


#include <c7format/format_cmn.hpp>
#include <c7strmbuf/strref.hpp>


#define C7_FORMAT_ANALYZED_FORMAT (1)


namespace c7::format_r1 {


using c7::format_cmn::format_item;
using c7::format_cmn::limited_vector;
using c7::format_cmn::analyze_format;
using c7::format_cmn::formatter;


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

    static analyzed_format get_for_literal(const char *fmt) {
	return analyzed_format{fmt};
    }

private:
    std::vector<format_item> fmts_;
};


// C7_FORMAT_LITERAL
inline analyzed_format operator""_F(const char *s, size_t)
{
    return analyzed_format::get_for_literal(s);
}


#include <c7format/format_api.hpp>	// public API


} // namespace c7::format_r1


#endif // format_r1.hpp
