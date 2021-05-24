/*
 * c7event.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7app.hpp>
#include <c7event/service.hpp>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <mutex>


namespace c7 {
namespace event {


/*----------------------------------------------------------------------------
                                 timer provider
----------------------------------------------------------------------------*/

class timer_provider: public provider_interface {
public:
    static auto make() {
	return std::unique_ptr<timer_provider>(new timer_provider());
    }

    ~timer_provider() override {
	(void)::close(fd_);
    }

    result<> init() {
	fd_ = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
	if (fd_ == C7_SYSERR) {
	    return c7result_err(errno, "timerfd_create() failed");
	}
	return c7result_ok();
    }

    result<> start(c7::usec_t beg, c7::usec_t interval,
		   std::function<void(timerid_t, uint64_t)>&& callback, bool is_abs) {
	::itimerspec itm;
	itm.it_value.tv_sec  = beg / C7_TIME_S_us;
	itm.it_value.tv_nsec = (beg % C7_TIME_S_us) * 1000;
	itm.it_interval.tv_sec  = interval/ C7_TIME_S_us;
	itm.it_interval.tv_nsec = (interval % C7_TIME_S_us) * 1000;
	
	int flags = is_abs ? TFD_TIMER_ABSTIME : 0;

	if (timerfd_settime(fd_, flags, &itm, nullptr) == C7_SYSERR) {
	    return c7result_err(errno, "timerfd_settime(fd:%{})", fd_);
	}

	count_ = (interval == 0) ? 1 : -1UL;
	callback_ = std::move(callback);
	return c7result_ok();
    }

    int fd() override {
	return fd_;
    }

    void on_event(monitor& mon, int, uint32_t events) override {
	uint64_t tmo_n;
	if (::read(fd_, &tmo_n, sizeof(tmo_n)) != sizeof(tmo_n)) {
	    // TODO: notify error
	    mon.unsubscribe(fd_);
	    return;
	}
	count_--;
	callback_(fd_, tmo_n);
	if (count_ == 0) {
	    mon.unsubscribe(fd_);
	}
    }

private:
    int fd_ = C7_SYSERR;
    uint64_t count_ = 0;
    std::function<void(timerid_t, uint64_t)> callback_;

    timer_provider() = default;
};


/*----------------------------------------------------------------------------
                                    monitor
----------------------------------------------------------------------------*/

monitor::monitor(monitor&& o): epfd_(o.epfd_), prvdic_(std::move(o.prvdic_))
{
    o.epfd_ = C7_SYSERR;
}

monitor::~monitor()
{
    (void)::close(epfd_);
}

result<> monitor::init()
{
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
		// TODO: notify error
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

result<timerid_t>
monitor::timer_start(c7::usec_t beg, c7::usec_t interval,
		     std::function<void(timerid_t, uint64_t)> callback, bool is_abs)
{
    auto timer = timer_provider::make();
    if (auto res = timer->init(); !res) {
	return c7result_err(std::move(res));
    }
    if (auto res = timer->start(beg, interval, std::move(callback), is_abs); !res) {
	return c7result_err(std::move(res));
    }
    auto timerid = timer->fd();
    if (auto res = subscribe(std::move(timer)); !res) {
	return c7result_err(std::move(res));
    }
    return c7result_ok(timerid);
}

result<> monitor::timer_restart(timerid_t timerid,
				c7::usec_t beg, c7::usec_t interval,
				std::function<void(timerid_t, uint64_t)> callback, bool is_abs)
{
    auto unlock = lock_.lock();
    auto it = prvdic_.find(timerid);
    if (it == prvdic_.end()) {
	return c7result_err(ENOENT, "timerid:%{} is not subscribed.", timerid);
    }
    auto timer = dynamic_cast<timer_provider*>((*it).second.s_ptr.get());
    return timer->start(beg, interval, std::move(callback), is_abs);
}

result<> monitor::timer_pause(timerid_t timerid)
{
    return suspend(timerid);
}

result<> monitor::timer_resume(timerid_t timerid)
{
    return resume(timerid);
}

result<> monitor::timer_stop(timerid_t timerid)
{
    return unsubscribe(timerid);
}


/*----------------------------------------------------------------------------
                              default event loop
----------------------------------------------------------------------------*/

static monitor EventLoop;


static std::once_flag once_init;

static void init()
{
    if (auto res = EventLoop.init(); !res) {
	c7error(res);
    }

    c7::thread::thread loop;
    loop.target([]() { EventLoop.loop(); });
    if (auto res = loop.start(); !res) {
	c7error(res);
    }
}

monitor& default_event_monitor()
{
    std::call_once(once_init, init);
    return EventLoop;
}

result<> subscribe(std::shared_ptr<provider_interface> provider, uint32_t events)
{
    return subscribe("", std::move(provider), events);
}

result<> subscribe(const std::string& key,
		   std::shared_ptr<provider_interface> provider, uint32_t events)
{
    std::call_once(once_init, init);
    return EventLoop.subscribe(key, std::move(provider), events);
}

result<> change_fd(int prvfd, uint new_prvfd)
{
    return EventLoop.change_fd(prvfd, new_prvfd);
}

result<> change_event(int prvfd, uint32_t events)
{
    return EventLoop.change_event(prvfd, events);
}

result<> suspend(int prvfd)
{
    return EventLoop.suspend(prvfd);
}

result<> resume(int prvfd)
{
    return EventLoop.resume(prvfd);
}

result<> unsubscribe(int prvfd)
{
    return EventLoop.unsubscribe(prvfd);
}

void forever()
{
    std::call_once(once_init, init);
    for (;;) {
	pause();
    }
}

result<timerid_t> timer_start(c7::usec_t beg, c7::usec_t interval,
			      std::function<void(timerid_t, uint64_t)> callback, bool is_abs)
{
    std::call_once(once_init, init);
    return EventLoop.timer_start(beg, interval, callback, is_abs);
}

result<> timer_restart(timerid_t timerid,
		       c7::usec_t beg, c7::usec_t interval,
		       std::function<void(timerid_t, uint64_t)> callback, bool is_abs)
{
    return EventLoop.timer_restart(timerid, beg, interval, callback, is_abs);
}

result<> timer_pause(timerid_t timerid)
{
    return EventLoop.timer_pause(timerid);
}

result<> timer_resume(timerid_t timerid)
{
    return EventLoop.timer_resume(timerid);
}

result<> timer_stop(timerid_t timerid)
{
    return EventLoop.timer_stop(timerid);
}


/*----------------------------------------------------------------------------
                                   I/O port
----------------------------------------------------------------------------*/

socket_port::socket_port(c7::socket&& sock): sock_(std::move(sock)) {}

socket_port::socket_port(socket_port&& o):
    sock_(std::move(o.sock_)), reverse_endian_(o.reverse_endian_)
{
    o.reverse_endian_ = false;
}	

socket_port& socket_port::operator=(socket_port&& o)
{
    if (this != &o) {
	sock_ = std::move(o.sock_);
	reverse_endian_ = o.reverse_endian_;
	o.reverse_endian_ = false;
    }
    return *this;
}

socket_port socket_port::tcp()
{
    if (auto res = c7::socket::tcp(); res) {
	return socket_port(std::move(res.value()));
    }
    return socket_port(c7::socket());
}

socket_port socket_port::unix()
{
    if (auto res = c7::socket::unix(); res) {
	return socket_port(std::move(res.value()));
    }
    return socket_port(c7::socket());
}

int socket_port::fd_number()
{
    return int(sock_);
}

bool socket_port::is_alive()
{
    return bool(sock_);
}

void socket_port::set_different_endian()
{
    reverse_endian_ = true;
}

bool socket_port::is_different_endian()
{
    return reverse_endian_;
}

result<> socket_port::set_nonblocking(bool enable)
{
    return sock_.set_nonblocking(enable);
}

result<socket_port> socket_port::accept()
{
    if (auto res = sock_.accept(); !res) {
	return c7result_err(std::move(res));
    } else {
	return c7result_ok(socket_port(std::move(res.value())));
    }
}

result<> socket_port::get_so_error(int *so_error)
{
    ::socklen_t so_size = sizeof(*so_error);
    return sock_.getsockopt(SOL_SOCKET, SO_ERROR, so_error, &so_size);
}

result<> socket_port::connect(const socket_addr& addr)
{
    if (auto un = std::get_if<std::string>(&addr); un) {
	return sock_.connect(*un);
    } else if (auto ip = std::get_if<::sockaddr_in>(&addr); ip) {
	return sock_.connect(*ip);
    } else {
	return c7result_err(EINVAL, "socket_addr is empty.");
    }
}

io_result socket_port::read_n(void *bufaddr, size_t req_n)
{
    return sock_.read_n(bufaddr, req_n);
}

io_result socket_port::write_v(::iovec*& iov_io, int& ioc_io)
{
    return sock_.write_v(iov_io, ioc_io);
}

void socket_port::close()
{
    sock_.close();
}

void socket_port::print(std::ostream& out, const std::string& spec) const
{
    sock_.print(out, spec);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace event;
} // namespace c7
