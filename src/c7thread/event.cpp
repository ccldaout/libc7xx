/*
 * c7thread/event.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7thread/event.hpp>
#include <c7utils/time.hpp>


namespace c7::thread {


event::event(bool init): c_(), event_(false)
{
}


bool event::is_set()
{
    return event_;
}


void event::set()
{
    c_.lock_notify_all([this](){ event_ = true; on_set(); });
}


void event::clear()
{
    c_.lock_notify_all([this](){ event_ = false; });
}


bool event::wait_unified(c7::usec_t timeout, bool clear)
{
    ::timespec *tmp = c7::mktimespec(timeout);
    auto defer = c_.lock();
    while (!event_) {
	if (!c_.wait(tmp))
	    return false;
    }
    if (clear)
	event_ = false;
    return true;
}


} // namespace c7::thread
