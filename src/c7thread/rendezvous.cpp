/*
 * c7thread/rendezvous.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7thread/rendezvous.hpp>
#include <c7utils/time.hpp>


namespace c7::thread {


rendezvous::rendezvous(int n_entry):
    c_(), id_(0), n_entry_(n_entry), waiting_(0)
{
}


bool rendezvous::wait(c7::usec_t timeout)
{
    ::timespec *tmp = c7::mktimespec(timeout);
    auto defer = c_.lock();
    if (++waiting_ == n_entry_) {
	id_++;
	waiting_ = 0;
	c_.notify_all();
    } else {
	auto cur_id = id_;
	while (n_entry_ > 0 && cur_id == id_) {
	    if (!c_.wait(tmp))
		return false;
	}
    }
    if (n_entry_ < 0)
	return false;		// aborting
    return true;		// all
}


void rendezvous::abort()
{
    auto defer = c_.lock();
    if (n_entry_ > 0) {
	n_entry_ *= -1;
	c_.notify_all();
    }
}


void rendezvous::reset()
{
    auto defer = c_.lock();
    if (n_entry_ < 0)
	n_entry_ *= -1;
    id_++;
    waiting_ = 0;
}


} // namespace c7::thread
