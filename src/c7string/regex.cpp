/*
 * c7string/regex.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <regex>
#include <c7string/regex.hpp>


namespace c7::str {


class regex::impl {
public:
    impl(std::string_view pattern, bool ignore_case);
    bool match(std::string_view s);
    bool match(std::string_view s, c7::strvec& submatch);

private:
    std::regex regex_;
    std::cmatch match_;
};


static auto make_flags(bool ignore_case)
{
    std::regex::flag_type flags = std::regex_constants::ECMAScript;
    if (ignore_case) {
	flags |= std::regex_constants::icase;
    }
    return flags;
}


regex::impl::impl(std::string_view pattern, bool ignore_case):
    regex_(pattern.data(), pattern.size(), make_flags(ignore_case))
{
}


bool regex::impl::match(std::string_view s)
{
    return std::regex_match(s.data(), s.data()+s.size(), match_, regex_);
}


bool regex::impl::match(std::string_view s, c7::strvec& submatch)
{
    if (!std::regex_match(s.data(), s.data()+s.size(), match_, regex_)) {
	return false;
    }

    submatch.clear();
    for (size_t i = 0; i < match_.size(); i++) {
	submatch += match_[i].str();
    }

    return true;
}


regex::regex(std::string_view pattern, bool ignore_case):
    pimpl_(std::make_unique<impl>(pattern, ignore_case))
{
}


regex::~regex() = default;


bool regex::match(std::string_view s)
{
    return pimpl_->match(s);
}


bool regex::match(std::string_view s, c7::strvec& submatch)
{
    return pimpl_->match(s, submatch);
}


} // namespace c7::str
