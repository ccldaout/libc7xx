/*
 * c7event/connector_impl.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_CONNECTOR_IMPL_HPP_LOADED__
#define C7_EVENT_CONNECTOR_IMPL_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/connector.hpp>
#include <c7event/receiver_impl.hpp>
#include <c7event/timer.hpp>


namespace c7::event {


// implementation of connector


template <typename Msgbuf, typename Port, typename Receiver>
connector<Msgbuf, Port, Receiver>::~connector() = default;


template <typename Msgbuf, typename Port, typename Receiver>
connector<Msgbuf, Port, Receiver>::connector(const sockaddr_gen& addr, service_ptr&& svc, provider_hint hint):
    provider_interface(), addr_(addr), svc_(std::move(svc)), hint_(hint)
{
    port_ = make_port();
}


template <typename Msgbuf, typename Port, typename Receiver>
int connector<Msgbuf, Port, Receiver>::fd()
{
    return port_.fd_number();
}


template <typename Msgbuf, typename Port, typename Receiver>
uint32_t connector<Msgbuf, Port, Receiver>::default_epoll_events()
{
    return EPOLLOUT;
}


template <typename Msgbuf, typename Port, typename Receiver>
void connector<Msgbuf, Port, Receiver>::on_manage(monitor& mon, int prvfd)
{
    port_.set_nonblocking(true);
    if (auto res = do_connect(mon); !res && !res.has_what(EINPROGRESS)) {
	start_timer(mon, prvfd);
    }
    // DON'T setup unmanage on close as follow in this provider.
    //
    // receiver::on_manage()
    //    port_.add_on_close([&mon, prvfd](){ mon.unmanage(prvfd); });
    //
    // When retrying connect, we must use new socket descriptor by system
    // requirements, and close old descriptor which was failed to connect.
    // If unmanage on close is enabled, *this* connector object is
    // deleted although it is needed to retry connect.
}


template <typename Msgbuf, typename Port, typename Receiver>
void connector<Msgbuf, Port, Receiver>::on_event(monitor& mon, int prvfd, uint32_t)
{
    int so_error;
    if (auto res = port_.get_so_error(&so_error); !res) {
	on_error(port_, res);
	mon.unmanage(prvfd);
	return;
    }
    if (so_error != 0) {
	on_error(port_, c7result_err(so_error));
	start_timer(mon, prvfd);
	return;
    }
	
    // SUCCESS
    port_.set_nonblocking(false);
    mon.change_provider(prvfd, make_receiver<service_interface<Msgbuf, Port>, Receiver>(std::move(port_), svc_, hint_));
    mon.change_event(prvfd, EPOLLIN);
}


template <typename Msgbuf, typename Port, typename Receiver>
void connector<Msgbuf, Port, Receiver>::start_timer(monitor& mon, int prvfd)
{
    mon.suspend(prvfd);

    c7::usec_t delay_us = delay_s_ * C7_TIME_S_us;
    delay_s_ += (delay_s_ >> 1);
    if (delay_s_ > 30) {
	delay_s_ = 30;
    }

    auto callback = [this, &mon](auto, auto){ retry_connect(mon); };
    auto res = timer_start(mon, delay_us, 0, std::move(callback));
    if (!res) {
	on_error(port_, res);
	mon.unmanage(prvfd);
    }
}


template <typename Msgbuf, typename Port, typename Receiver>
void connector<Msgbuf, Port, Receiver>::retry_connect(monitor& mon)
{
    // [BUG] Following original codes has potential problem on multi thread.
    //       Old descriptor of port_ is closed by assignment at [1]. If other
    //       thread create descriptor and manage it before [2], mon.change_fd
    //       cause broken association of descriptor and provider object.
    //
    //   [0] auto oldfd = port_.fd_number();
    //   [1] port_ = make_port();
    //   [2] mon.change_fd(oldfd, port_.fd_number());

    auto new_port = make_port();
    mon.change_fd(port_.fd_number(), new_port.fd_number());
    port_ = std::move(new_port);
    port_.set_nonblocking(true);

    auto prvfd = port_.fd_number();

    if (auto res = do_connect(mon); !res && !res.has_what(EINPROGRESS)) {
	start_timer(mon, prvfd);
    } else {
	mon.resume(prvfd);
    }
}


template <typename Msgbuf, typename Port, typename Receiver>
result<> connector<Msgbuf, Port, Receiver>::do_connect(monitor& mon)
{
    svc_->on_pre_connect(mon, port_);
    return port_.connect(addr_);
}


template <typename Msgbuf, typename Port, typename Receiver>
Port connector<Msgbuf, Port, Receiver>::make_port()
{
    if (addr_.is_ipv4()) {
	return Port::tcp();
    } else if (addr_.is_unix()) {
	return Port::unix();
    } else {
	return Port();
    }
}


} // namespace c7::event


#endif // c7event/connector_impl.hpp
