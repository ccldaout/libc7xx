/*
 * c7utils/passwd.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_UTILS_PASSWD_HPP_LOADED_
#define C7_UTILS_PASSWD_HPP_LOADED_
#include <c7common.hpp>


#include <pwd.h>
#include <c7result.hpp>
#include <c7utils/memory.hpp>


namespace c7 {


class passwd {
public:
    using passwd_ptr = unique_cptr<::passwd>;

private:
    passwd_ptr pwd_;

public:
    passwd() = default;

    explicit passwd(passwd_ptr&& pwd): pwd_(std::move(pwd)) {}

    ::passwd* operator->() {
	return pwd_.get();
    }

    ::passwd& operator*() {
	return *pwd_;
    }

    static c7::result<passwd> by_name(const std::string& name);
    static c7::result<passwd> by_uid(::uid_t uid);
};


} // namespace c7


#endif // c7utils/passwd.hpp
