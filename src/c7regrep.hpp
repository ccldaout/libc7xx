/*
 * c7regrep.hpp
 *
 * Copyright (c) 2025 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_REGREP_HPP_LOADED_
#define C7_REGREP_HPP_LOADED_
#include <c7common.hpp>


#include <memory>
#include <regex>


#define C7_REGREP_MATCH_CNT	(10)

#define C7_REGREP_HIGHLIGHT	(1U<<0)
#define C7_REGREP_EVAL_CBSS	(1U<<1)		// C backslash sequence
#define C7_REGREP_OLDRULE	(1U<<2)
#define C7_REGREP_RULEONLY	(1U<<3)


namespace c7 {


class regrep {
public:
    regrep();
    ~regrep();
    regrep(regrep&&) = default;
    regrep& operator=(regrep&&) = default;

    c7::result<> init(const std::string& reg_pat,
		      const std::string& repl_rule,
		      std::regex::flag_type regex_flag,
		      uint32_t regrep_flag);

    c7::result<bool> exec(const std::string& s, std::string& out) {
	return exec(s.c_str(), out);
    }

    c7::result<bool> exec(const char *in, std::string& out);

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};


} // namespace c7


#endif // c7regrep.hpp
