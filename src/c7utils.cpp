/*
 * c7utils.cpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7defer.hpp>
#include <c7utils.hpp>
#include <algorithm>
#include <cstdlib>


namespace c7 {


c7::usec_t time_us()
{
    ::timeval tv;
    (void)::gettimeofday(&tv, nullptr);
    return ((c7::usec_t)tv.tv_sec) * C7_TIME_S_us + tv.tv_usec;
}


c7::usec_t sleep_us(c7::usec_t duration)
{
    ::timespec req, rem;
    req.tv_sec  =  duration / C7_TIME_S_us;
    req.tv_nsec = (duration % C7_TIME_S_us) * 1000;
    if (::nanosleep(&req, &rem) == C7_SYSERR) {
	return C7_SYSERR;
    }
    duration = (rem.tv_sec * C7_TIME_S_us) + (rem.tv_nsec + 500) / 1000;
    return std::max(duration, (c7::usec_t)0);
}


::timespec *timespec_from_duration(c7::usec_t duration, c7::usec_t reftime)
{
    static thread_local ::timespec abstime;
    if (reftime == 0)
	reftime = c7::time_us();
    reftime += duration;
    abstime.tv_sec =   reftime / C7_TIME_S_us;
    abstime.tv_nsec = (reftime % C7_TIME_S_us) * 1000;
    return &abstime;
}


template <typename T>
static result<passwd>
getpw_by_key(T key, int (*getpw)(T, ::passwd*, char*, size_t, ::passwd **), const char *type)
{
    auto pwd = c7::make_unique_cptr<::passwd>();

    int bufsize = 0;

    for (;;) {
	bufsize += 128;
	auto np = std::realloc(pwd.get(), sizeof(*pwd) + bufsize);
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
