/*
 * format_r4.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_FORMAT_R4_HPP_LOADED_
#define C7_FORMAT_R4_HPP_LOADED_
#include <c7common.hpp>


#include <c7format/format_cmn.hpp>
#include <c7strmbuf/strref.hpp>
#include <c7utils/spinlock.hpp>


#define C7_FORMAT_ANALYZED_FORMAT 	(1)
#define C7_FORMAT_LITERAL		(1)


namespace c7::format_r4 {


using c7::format_cmn::format_item;
using c7::format_cmn::analyze_format;
using c7::format_cmn::formatter;


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
	c7::spinlock lock(dic_lock_);
	const auto& [it, inserted] = p_dic_.try_emplace(fmt);
	lock.release();
	if (inserted) {
	    (*it).second.init(fmt);
	}
	return (*it).second;
    }

private:
    std::vector<format_item> fmts_;

    static std::unordered_map<const void*, analyzed_format> p_dic_;
    static volatile int dic_lock_;

    void init(const char *s) {
	analyze_format(s, fmts_);
    }
};


// C7_FORMAT_LITERAL
inline const analyzed_format& operator""_F(const char *s, size_t)
{
    return analyzed_format::get_for_literal(s);
}


#include <c7format/format_api.hpp>	// public API


} // namespace c7::format_r4


#endif // format_r4.hpp
