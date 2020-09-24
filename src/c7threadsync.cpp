/*
 * c7threadsync.cpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7threadsync.hpp>


namespace c7 {
namespace thread {


/*----------------------------------------------------------------------------
                                 thread group
----------------------------------------------------------------------------*/

class group::impl {
private:
    enum flag { WAIT, START, ABORT };
    condvar cv_;
    flag flag_;
    int unfinished_;
    std::vector<proxy> threads_;

    void on_start(proxy) {
	auto defer = cv_.lock();
	while (flag_ == WAIT) {
	    cv_.wait();
	}
	if (flag_ == ABORT) {
	    self::abort();
	}
    }

    void on_finish(proxy th) {
	auto defer = cv_.lock();
	unfinished_--;
	if (flag_ == START) {
	    on_any_finish(th);
	}
	if (unfinished_ == 0) {
	    if (flag_ == START) {
		on_all_finish();
	    }
	    cv_.notify_all();
	}
    }

public:
    c7::delegate<void, proxy> on_any_finish;
    c7::delegate<void       > on_all_finish;

    impl(const impl&) = delete;
    impl(impl&&) =delete;
    impl& operator=(const impl&) = delete;
    impl& operator=(impl&&) = delete;

    impl():
	cv_(), flag_(WAIT), unfinished_(0) {
    }

    result<void> monitor(thread& th) {
	auto defer = cv_.lock();
	if (flag_ == ABORT) {
	    return c7result_err("Already aborted: some thread cannot be started previously.");
	}
	th.on_start  += [this](proxy p) { return on_start(p); };
	th.on_finish += [this](proxy p) { return on_finish(p); };
	if (auto res = th.start(); !res) {
	    flag_ = ABORT;
	    cv_.notify_all();
	    return res;
	}
	threads_.push_back(proxy(th));
	unfinished_++;
	return c7result_ok();
    }

    result<void> start() {
	auto defer = cv_.lock();
	if (flag_ == ABORT) {
	    return c7result_err("Already aborted: some thread cannot be started previously.");
	}
	flag_ = START;
	cv_.notify_all();
	return c7result_ok();
    }

    void wait() {
	auto defer = cv_.lock();
	cv_.wait_for([this](){ return unfinished_ == 0; });
    }

    size_t size() {
	return threads_.size();
    }
    
    auto begin() {
	return threads_.begin();
    }

    auto end() {
	return threads_.end();
    }
};

group::group():
    pimpl(new group::impl()),
    on_any_finish(pimpl->on_any_finish), on_all_finish(pimpl->on_all_finish)
{
}

group::group(group&& o):
    pimpl(o.pimpl), 
    on_any_finish(pimpl->on_any_finish), on_all_finish(pimpl->on_all_finish)
{
    o.pimpl = nullptr;
}

group& group::operator=(group&& o)
{
    if (this != &o) {
	delete pimpl;
	pimpl = o.pimpl;
	o.pimpl = nullptr;
	on_any_finish = std::move(o.on_any_finish);
	on_all_finish = std::move(o.on_all_finish);
    }
    return *this;
}

group::~group()
{
    delete pimpl;
    pimpl = nullptr;
}

result<void> group::add(thread& th)
{
    return pimpl->monitor(th);
}

result<void> group::start()
{
    return pimpl->start();
}

void group::wait()
{
    pimpl->wait();
}

size_t group::size()
{
    return pimpl->size();
}

group::iterator group::begin()
{
    return pimpl->begin();
}

group::iterator group::end()
{
    return pimpl->end();
}


/*----------------------------------------------------------------------------
                            synchronization tools
----------------------------------------------------------------------------*/

// counter

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
    counter_ = count;
}

void counter::up()
{
    c_._lock();
    counter_++;
    c_.notify_all();
    c_.unlock();
}

bool counter::down(c7::usec_t timeout)
{
    ::timespec *tmp = c7::mktimespec(timeout);
    auto defer = c_.lock();
    while (counter_ == 0) {
	if (!c_.wait(tmp))
	    return false;
    }
    if (--counter_ == 0) {
	on_zero();
    }
    c_.notify_all();
    return true;
}

bool counter::wait_zero(c7::usec_t timeout)
{
    ::timespec *tmp = c7::mktimespec(timeout);
    auto defer = c_.lock();
    while (counter_ != 0) {
	if (!c_.wait(tmp))
	    return false;
    }
    return true;
}

// event

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

// mask

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

// rendezvous

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


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace thread
} // namespace c7
