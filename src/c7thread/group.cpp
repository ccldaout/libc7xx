/*
 * c7thread/group.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7thread/group.hpp>
#include <c7thread/condvar.hpp>


namespace c7::thread {


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

    result<> monitor(thread& th) {
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

    result<> start() {
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


result<> group::add(thread& th)
{
    return pimpl->monitor(th);
}


result<> group::start()
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


} // namespace c7::thread
