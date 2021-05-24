/*
 * c7event/monitor.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_MONITOR_HPP_LOADED__
#define C7_EVENT_MONITOR_HPP_LOADED__
#include <c7common.hpp>


#include <c7thread.hpp>
#include <c7utils.hpp>
#include <sys/epoll.h>
#include <memory>


namespace c7 {
namespace event {


/*----------------------------------------------------------------------------
                             event loop interface
----------------------------------------------------------------------------*/

typedef int timerid_t;
class monitor;


class provider_interface {
public:
    provider_interface(const provider_interface&) = delete;
    provider_interface& operator=(const provider_interface&) = delete;
    provider_interface(provider_interface&&) = delete;
    provider_interface& operator=(provider_interface&&) = delete;

    virtual ~provider_interface() = default;
    virtual int fd() = 0;
    virtual uint32_t default_epoll_events() { return EPOLLIN; };
    virtual void on_subscribed(monitor&, int prvfd) {}
    virtual void on_event(monitor&, int prvfd, uint32_t events) = 0;
    virtual void on_unsubscribed(monitor&, int prvfd) {}

protected:
    provider_interface() = default;
};


class monitor {
public:
    monitor(const monitor&) = delete;
    monitor& operator=(const monitor&) = delete;

    monitor() = default;
    monitor(monitor&&);
    ~monitor();

    result<> init();
    void loop();

    result<> subscribe(std::shared_ptr<provider_interface> provider, uint32_t events = 0);
    result<> subscribe(const std::string& key, std::shared_ptr<provider_interface> provider, uint32_t events = 0);
    result<> change_fd(int prvfd, int new_prvfd);
    result<> change_event(int prvfd, uint32_t events);
    result<> change_provider(int prvfd, std::shared_ptr<provider_interface> provider);
    result<> suspend(int prvfd);
    result<> resume(int prvfd);
    result<> unsubscribe(int prvfd);
    template <typename T> result<std::shared_ptr<T>> find(const std::string& key);
    
    result<timerid_t> timer_start(c7::usec_t beg, c7::usec_t interval,
				  std::function<void(timerid_t, uint64_t)> callback,
				  bool is_abs = false);
    result<> timer_restart(timerid_t,
			   c7::usec_t beg_absolute, c7::usec_t interval,
			   std::function<void(timerid_t, uint64_t)> callback,
			   bool is_abs = false);
    result<> timer_pause(timerid_t);
    result<> timer_resume(timerid_t);
    result<> timer_stop(timerid_t);

private:
    struct provider_info {
	provider_info() = default;
	provider_info(provider_info&&) = default;
	provider_info& operator=(provider_info&&) = default;
	provider_info(std::shared_ptr<provider_interface> prv, uint32_t events):
	    s_ptr(std::move(prv)), events(events) {}

	std::shared_ptr<provider_interface> s_ptr;
	uint32_t events;
    };

    int epfd_ = C7_SYSERR;
    std::unordered_map<int, provider_info> prvdic_;
    std::unordered_map<std::string, std::weak_ptr<provider_interface>> keyprvdic_;
    c7::thread::mutex lock_;

    result<std::shared_ptr<provider_interface>> find_provider(const std::string& key);
};

template <typename T>
result<std::shared_ptr<T>> monitor::find(const std::string& key)
{
    if (auto res = find_provider(key); !res) {
	return res;
    } else if (auto sp = std::dynamic_pointer_cast<T>(res.value()); !sp) {
	return c7result_err(EINVAL, "downcast failed: key: %{}", key);
    } else {
	return c7result_ok(std::move(sp));
    }
}
    

// default monitor interfaces

monitor& default_event_monitor();
result<> subscribe(std::shared_ptr<provider_interface> provider, uint32_t events = 0);
result<> subscribe(const std::string& key, std::shared_ptr<provider_interface> provider, uint32_t events = 0);
result<> change_fd(int prvfd, int new_prvfd);
result<> change_event(int prvfd, uint32_t events);
result<> change_provider(int prvfd, std::shared_ptr<provider_interface> provider);
result<> suspend(int prvfd);
result<> resume(int prvfd);
result<> unsubscribe(int prvfd);
template <typename T>
result<std::shared_ptr<T>> find(const std::string& key)
{
    return default_event_monitor().find<T>(key);
}
void forever();
    
result<timerid_t> timer_start(c7::usec_t beg_absolute, c7::usec_t interval,
			      std::function<void(timerid_t, uint64_t)> callback,
			      bool is_abs = false);
result<> timer_restart(timerid_t,
		       c7::usec_t beg_absolute, c7::usec_t interval,
		       std::function<void(timerid_t, uint64_t)> callback,
		       bool is_abs = false);
result<> timer_pause(timerid_t);
result<> timer_resume(timerid_t);
result<> timer_stop(timerid_t);


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::event
} // namespace c7


#endif // c7event/monitor.hpp
