/*
 * c7thread/counter.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7thread/counter.hpp>
#include <c7utils.hpp>


namespace c7::thread {


counter::counter(int init_count):
    c_(), counter_(init_count)
{
}

    
int counter::count()
{
    return counter_;
}


void counter::reset(int count)
{
    c_._lock();
    counter_ = count;
    c_.notify_all();
    c_.unlock();
}


void counter::up(int n)
{
    c_._lock();
    counter_ += n;
    c_.notify_all();
    c_.unlock();
}


bool counter::down(c7::usec_t timeout)
{
    ::timespec *tmp = c7::mktimespec(timeout);
    auto defer = c_.lock();
    while (counter_ == 0) {
	if (!c_.wait(tmp)) {
	    return false;
	}
    }
    if (--counter_ == 0) {
	on_zero();
    }
    c_.notify_all();
    return true;
}


bool counter::wait_atleast(int expect, c7::usec_t timeout)
{
    ::timespec *tmp = c7::mktimespec(timeout);
    auto defer = c_.lock();
    while (counter_ < expect) {
	if (!c_.wait(tmp)) {
	    return false;
	}
    }
    return true;
}


bool counter::wait_just(int expect, c7::usec_t timeout)
{
    ::timespec *tmp = c7::mktimespec(timeout);
    auto defer = c_.lock();
    while (counter_ != expect) {
	if (!c_.wait(tmp)) {
	    return false;
	}
    }
    return true;
}


} // namespace c7::thread
