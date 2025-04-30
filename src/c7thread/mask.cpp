/*
 * c7thread/mask.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7thread/mask.hpp>
#include <c7utils/time.hpp>


namespace c7::thread {


mask::mask(uint64_t ini_mask): c_(), mask_(0) {}


uint64_t mask::get()
{
    return mask_;
}


void mask::clear()
{
    c_.lock_notify_all([this](){ mask_ = 0; });
}


void mask::on(uint64_t on_mask)
{
    c_.lock_notify_all([this, on_mask](){ mask_ |= on_mask; });
}


void mask::off(uint64_t off_mask)
{
    c_.lock_notify_all([this, off_mask](){ mask_ &= ~off_mask; });
}


bool mask::change(std::function<void(uint64_t& in_out)> func)
{
    auto defer = c_.lock();
    uint64_t savemask = mask_;
    func(mask_);
    if (savemask != mask_)
	c_.notify_all();
    return (savemask != mask_);
}


uint64_t mask::wait(std::function<uint64_t(uint64_t& in_out)> func, c7::usec_t timeout)
{
    ::timespec *tmp = c7::mktimespec(timeout);
    auto defer = c_.lock();
    for (;;) {
	uint64_t savemask = mask_;
	uint64_t ret = func(mask_);
	if (ret != 0) {
	    if (mask_ != savemask)
		c_.notify_all();
	    return ret;
	}
	mask_ = savemask;
	if (!c_.wait(tmp))
	    return 0;
    }
}


} // namespace c7::thread
