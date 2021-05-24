/*
 * c7utils.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7defer.hpp>
#include <c7format.hpp>
#include <c7utils.hpp>
#include <algorithm>
#include <cstdlib>
#include <cstring>


namespace c7 {


/*----------------------------------------------------------------------------
                                     time
----------------------------------------------------------------------------*/

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


/*----------------------------------------------------------------------------
                              simple raw storage
----------------------------------------------------------------------------*/

storage::~storage()
{
    std::free(addr_);
}

storage::storage(storage&& o)
{
    addr_ = o.addr_;
    size_ = o.size_;
    align_ = o.align_;
    o.addr_ = nullptr;
    o.size_ = 0;
}

storage& storage::operator=(storage&& o)
{
    if (this != &o) {
	std::free(addr_);
	addr_ = o.addr_;
	size_ = o.size_;
	align_ = o.align_;
	o.addr_ = nullptr;
	o.size_ = 0;
    }
    return *this;
}

storage& storage::copy_from(const storage& o)
{
    if (this != &o) {
	if (auto res = reserve(o.size_); !res) {
	    throw std::runtime_error(c7::format("%{}", res));
	}
	std::memcpy(addr_, o.addr_, o.size_);
    }
    return *this;
}

storage storage::copy_to()
{
    storage new_storage{align_};
    new_storage.copy_from(*this);
    return new_storage;
}

result<> storage::reserve(size_t req)
{
    auto new_size = c7_align(req, align_);
    if (size_ < new_size) {
	if (void *new_addr = std::realloc(addr_, new_size); new_addr == nullptr) {
	    return c7result_err(errno, "cannot allocate memory");
	} else {
	    addr_ = new_addr;
	    size_ = new_size;
	}
    }
    return c7result_ok();
}

void storage::reset()
{
    std::free(addr_);
    addr_ = nullptr;
    size_ = 0;
}

void storage::clear()
{
    std::memset(addr_, 0, size_);
}


} // namespace c7
