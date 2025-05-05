/*
 * c7string/regex.hpp
 *
 * Copyright (c) 2025 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=336895331
 */
#ifndef C7_STRING_REGEX_HPP_LOADED_
#define C7_STRING_REGEX_HPP_LOADED_
#include <c7common.hpp>


#include <memory>
#include <string_view>
#include <c7string/strvec.hpp>


#define C7_STRING_REGMATCH	(1U)


namespace c7::str {


// regex: C7_STRING_REGMATCH

class regex {
public:
    explicit regex(std::string_view pattern, bool ignore_case = false);
    ~regex();
    bool match(std::string_view s);
    bool match(std::string_view s, c7::strvec& submatch);

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};


} // namespace c7::str


#endif // c7string/regex.hpp
