/*
 * c7event/monitor.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7app.hpp>
#include <c7event/monitor.hpp>
#include <c7signal.hpp>
#include <unistd.h>
#include <mutex>		// once_flag


namespace c7::event {


/*----------------------------------------------------------------------------
                                    monitor
----------------------------------------------------------------------------*/

monitor::monitor(monitor&& o): epfd_(o.epfd_), prvdic_(std::move(o.prvdic_))
{
    o.epfd_ = C7_SYSERR;
}

monitor::~monitor()
{
    unsubscribe_all();
    ::close(epfd_);
}

result<> monitor::init()
{
    c7::signal::handle(SIGPIPE, SIG_IGN);
    epfd_ = ::epoll_create1(EPOLL_CLOEXEC);
    if (epfd_ == C7_SYSERR) {
	return c7result_err(errno, "epoll_create1() failed");
    }
    return c7result_ok();
}

void monitor::loop()
{
    for (;;) {
	::epoll_event evts[8];
	int ret = ::epoll_wait(epfd_, evts, c7_numberof(evts), -1);
	if (ret > 0) {
	    for (int i = 0; i < ret; i++) {
		auto ev = evts[i];
		// IMPORTANT: It's possible that a provider has been unsubscribed here.
		//            So, it is important to check the exsitency of provider and
		//            hold it to prevent some thread from freeing the provider.
		std::shared_ptr<provider_interface> hold;
		{
		    auto unlock = lock_.lock();
		    auto it = prvdic_.find(ev.data.fd);
		    if (it == prvdic_.end()) {
			continue;
		    }
		    hold = (*it).second.s_ptr;	// to prevent other thread from freeing the provider
		}
		hold->on_event(*this, ev.data.fd, ev.events);
	    }
	} else if (ret == C7_SYSERR) {
	    if (errno != EINTR) {
		// FATAL ERROR
		c7abort(c7result_err(errno, "epoll_wait() failed"));
	    }
	}
    }
}

result<> monitor::subscribe(std::shared_ptr<provider_interface> provider, uint32_t events)
{
    return subscribe("", std::move(provider), events);
}

result<> monitor::subscribe(const std::string& key,
			    std::shared_ptr<provider_interface> provider, uint32_t events)
{
    auto unlock = lock_.lock();
    int prvfd = provider->fd();

    if (!key.empty()) {
	if (auto it = keyprvdic_.find(key); it != keyprvdic_.end()) {
	    if (!(*it).second.expired()) {
		return c7result_err(EEXIST, "subscribe failed: duplicate key: %{}", key);
	    }
	}
    }
    
    if (events == 0) {
	events = provider->default_epoll_events();
    }
    // [MEMO] We use EPOLLHUP flag for other purpose; suspended flag. 
    //        EPOLLHUP is only used as output flag of epoll_wait in system call,
    //        so, that usage is safety.
    events &= ~EPOLLHUP;

    ::epoll_event ev;
    ev.events = events;
    ev.data.fd = prvfd;
    if (::epoll_ctl(epfd_, EPOLL_CTL_ADD, prvfd, &ev) == C7_SYSERR) {
	return c7result_err(errno, "epoll_ctl(ADD, %{}) failed", prvfd);
    }

    auto sp = provider.get();
    provider_info prv_info { std::move(provider), events };
    auto [it, _] = prvdic_.insert_or_assign(prvfd, std::move(prv_info));

    if (!key.empty()) {
	std::weak_ptr<provider_interface> wp = (*it).second.s_ptr;
	keyprvdic_.insert_or_assign(key, std::move(wp));
    }

    unlock();

    sp->on_subscribed(*this, prvfd);
    return c7result_ok();
}

result<> monitor::change_fd(int prvfd, int new_prvfd)
{
    if (prvfd == new_prvfd) {
	return c7result_err(EINVAL, "change_fd: same fd is specified");
    }

    auto unlock = lock_.lock();
    auto it = prvdic_.find(prvfd);
    if (it == prvdic_.end()) {
	return c7result_err(ENOENT, "prvfd:%{} is not subscribed.", prvfd);
    }
    auto prv_info = std::move((*it).second);

    if ((prv_info.events & EPOLLHUP) == 0) {		// not suspended
	::epoll_ctl(epfd_, EPOLL_CTL_DEL, prvfd, nullptr);
	::epoll_event ev;
	ev.events = prv_info.events;
	ev.data.fd = new_prvfd;
	if (::epoll_ctl(epfd_, EPOLL_CTL_ADD, new_prvfd, &ev) == C7_SYSERR) {
	    return c7result_err(errno, "epoll_ctl(ADD, %{}) failed", new_prvfd);
	}
    }

    prvdic_.insert_or_assign(new_prvfd, std::move(prv_info));
    prvdic_.erase(it);

    return c7result_ok();
}

result<> monitor::change_event(int prvfd, uint32_t events)
{
    auto unlock = lock_.lock();
    auto it = prvdic_.find(prvfd);
    if (it == prvdic_.end()) {
	return c7result_err(ENOENT, "prvfd:%{} is not subscribed.", prvfd);
    }
    events &= ~EPOLLHUP;
    events |= ((*it).second.events & EPOLLHUP);
    (*it).second.events = events;

    if ((events & EPOLLHUP) == 0) {			// not suspended
	::epoll_event ev;
	ev.events = events;
	ev.data.fd = prvfd;
	if (::epoll_ctl(epfd_, EPOLL_CTL_MOD, prvfd, &ev) == C7_SYSERR) {
	    return c7result_err(errno, "epoll_ctl(MOD, %{}) failed", prvfd);
	}
    }
    return c7result_ok();
}

result<> monitor::change_provider(int prvfd, std::shared_ptr<provider_interface> provider)
{
    auto unlock = lock_.lock();
    auto it = prvdic_.find(prvfd);
    if (it == prvdic_.end()) {
	return c7result_err(ENOENT, "prvfd:%{} is not subscribed.", prvfd);
    }
    auto sp = provider.get();
    (*it).second.s_ptr = std::move(provider);
    unlock();

    sp->on_subscribed(*this, prvfd);
    return c7result_ok();
}

result<> monitor::suspend(int prvfd)
{
    auto unlock = lock_.lock();
    auto it = prvdic_.find(prvfd);
    if (it == prvdic_.end()) {
	return c7result_err(ENOENT, "prvfd:%{} is not subscribed.", prvfd);
    }

    if (::epoll_ctl(epfd_, EPOLL_CTL_DEL, prvfd, nullptr) == C7_SYSERR) {
	return c7result_err(errno, "epoll_ctl(DEL, %{}) failed", prvfd);
    } 
    (*it).second.events |= EPOLLHUP;			// for suspended flag

    return c7result_ok();
}

result<> monitor::resume(int prvfd)
{
    auto unlock = lock_.lock();
    auto it = prvdic_.find(prvfd);
    if (it == prvdic_.end()) {
	return c7result_err(ENOENT, "prvfd:%{} is not subscribed.", prvfd);
    }
    (*it).second.events &= ~EPOLLHUP;			// resumed

    ::epoll_event ev;
    ev.events = (*it).second.events;
    ev.data.fd = prvfd;
    if (::epoll_ctl(epfd_, EPOLL_CTL_ADD, prvfd, &ev) == C7_SYSERR) {
	return c7result_err(errno, "epoll_ctl(ADD, %{}) failed", prvfd);
    }


    return c7result_ok();
}

result<> monitor::unsubscribe(int prvfd)
{
    auto unlock = lock_.lock();
    auto it = prvdic_.find(prvfd);
    if (it == prvdic_.end()) {
	return c7result_err(ENOENT, "prvfd:%{} is not subscribed.", prvfd);
    }

    auto provider = std::move((*it).second.s_ptr);
    prvdic_.erase(it);

    if (::epoll_ctl(epfd_, EPOLL_CTL_DEL, prvfd, nullptr) == C7_SYSERR) {
	return c7result_err(errno, "epoll_ctl(DEL, %{}) failed", prvfd);
    } 
    unlock();

    provider->on_unsubscribed(*this, prvfd);
    return c7result_ok();
}

void monitor::unsubscribe_all()
{
    // 1s step: notify unsubscribed by external to all provider.
    for (auto& [prvfd, pinfo]: prvdic_) {
	auto& provider = pinfo.s_ptr;
	::epoll_ctl(epfd_, EPOLL_CTL_DEL, prvfd, nullptr);
	provider->on_unsubscribed(*this, prvfd);
    }

    // 2st step: move prvdic_ to tmp in order to keep all provider in clearing prvdic_
    std::unordered_map<int, provider_info> tmp(std::move(prvdic_));;
    // 3rd step: all provider are released in destructing tmp.
}

result<std::shared_ptr<provider_interface>> monitor::find_provider(const std::string& key)
{
    auto unlock = lock_.lock();
    auto it = keyprvdic_.find(key);
    if (it == keyprvdic_.end()) {
	return c7result_err(ENOENT, "provider not found: key: %{}", key);
    }
    auto sp = (*it).second.lock();
    if (sp == nullptr) {
	keyprvdic_.erase(it);
	return c7result_err(ENOENT, "provider is already unsubscribed: key: %{}", key);
    }
    return c7result_ok(std::move(sp));
}


/*----------------------------------------------------------------------------
                              default event loop
----------------------------------------------------------------------------*/

static monitor default_monitor;
static c7::thread::thread default_thread;
static std::once_flag once_init;

static void init()
{
    if (auto res = default_monitor.init(); !res) {
	c7error(res);
    }

    default_thread.target([]() { default_monitor.loop(); });
    if (auto res = default_thread.start(); !res) {
	c7error(res);
    }
}

monitor& default_event_monitor()
{
    std::call_once(once_init, init);
    return default_monitor;
}

result<> subscribe(std::shared_ptr<provider_interface> provider, uint32_t events)
{
    return subscribe("", std::move(provider), events);
}

result<> subscribe(const std::string& key,
		   std::shared_ptr<provider_interface> provider, uint32_t events)
{
    std::call_once(once_init, init);
    return default_monitor.subscribe(key, std::move(provider), events);
}

result<> change_fd(int prvfd, uint new_prvfd)
{
    return default_monitor.change_fd(prvfd, new_prvfd);
}

result<> change_event(int prvfd, uint32_t events)
{
    return default_monitor.change_event(prvfd, events);
}

result<> suspend(int prvfd)
{
    return default_monitor.suspend(prvfd);
}

result<> resume(int prvfd)
{
    return default_monitor.resume(prvfd);
}

result<> unsubscribe(int prvfd)
{
    return default_monitor.unsubscribe(prvfd);
}

void forever()
{
    std::call_once(once_init, init);
    default_thread.join();
    if (default_thread.status() != c7::thread::thread::EXIT) {
	c7echo(default_thread.terminate_result(), "event loop thread has been crashed.");
    }
}


} // namespace c7::event
