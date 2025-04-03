/*
 * c7utils/passwd.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


//#include <c7defer.hpp>
//#include <c7format.hpp>
#include <c7utils/passwd.hpp>
//#include <algorithm>
#include <cstdlib>
//#include <cstring>


namespace c7 {


/*----------------------------------------------------------------------------
                             get passwd user data
----------------------------------------------------------------------------*/

template <typename T>
static result<passwd>
getpw_by_key(T key, int (*getpw)(T, ::passwd*, char*, size_t, ::passwd **), const char *type)
{
    auto pwd = c7::make_unique_cptr<::passwd>();

    int bufsize = 0;

    for (;;) {
	bufsize += 128;
	auto np = std::malloc(sizeof(*pwd) + bufsize);
	if (np == nullptr) {
	    return c7result_err(errno, "getpw%{}_r: realloc failed '%{}'", type, key);
	}
	pwd.reset(static_cast<::passwd*>(np));

	::passwd *result;
	int err = (*getpw)(key, pwd.get(), (char *)(pwd.get() + 1), bufsize, &result);
	if (err == 0 && result != nullptr) {
	    return c7result_ok(passwd(std::move(pwd)));
	}
	if (err == 0 && result == nullptr) {
	    return c7result_err(ENOENT, "getpw%{}_r: no entry '%{}'", type, key);
	}
	if (err != ERANGE) {
	    return c7result_err(err, "getpw%{}_r: failed '%{}'", type, key);
	}
	// Not enough buffer space, retry with extended buffer.
    }
}

result<passwd> passwd::by_name(const std::string& name)
{
    return getpw_by_key(name.c_str(), ::getpwnam_r, "nam");
}

result<passwd> passwd::by_uid(::uid_t uid)
{
    return getpw_by_key(uid, ::getpwuid_r, "uid");
}


} // namespace c7
