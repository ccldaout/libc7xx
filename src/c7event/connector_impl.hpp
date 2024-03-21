/*
 * c7event/connector_impl.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_CONNECTOR_IMPL_HPP_LOADED__
#define C7_EVENT_CONNECTOR_IMPL_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/connector.hpp>
#include <c7event/receiver_impl.hpp>
#include <c7event/timer.hpp>


namespace c7::event {


// implementation of connector


template <typename Msgbuf, typename Port>
connector<Msgbuf, Port>::~connector() = default;


template <typename Msgbuf, typename Port>
connector<Msgbuf, Port>::connector(const sockaddr_gen& addr, service_ptr&& svc, provider_hint hint):
    provider_interface(), addr_(addr), svc_(std::move(svc)), hint_(hint)
{
    port_ = make_port();
}


template <typename Msgbuf, typename Port>
int connector<Msgbuf, Port>::fd()
{
    return port_.fd_number();
}


template <typename Msgbuf, typename Port>
uint32_t connector<Msgbuf, Port>::default_epoll_events()
{
    return EPOLLOUT;
}


template <typename Msgbuf, typename Port>
void connector<Msgbuf, Port>::on_manage(monitor& mon, int prvfd)
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


template <typename Msgbuf, typename Port>
void connector<Msgbuf, Port>::on_event(monitor& mon, int prvfd, uint32_t)
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
    auto rcv = make_receiver(std::move(port_), svc_, hint_);
    mon.change_provider(prvfd, std::move(rcv));
    mon.change_event(prvfd, EPOLLIN);
}


template <typename Msgbuf, typename Port>
void connector<Msgbuf, Port>::start_timer(monitor& mon, int prvfd)
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


template <typename Msgbuf, typename Port>
void connector<Msgbuf, Port>::retry_connect(monitor& mon)
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


template <typename Msgbuf, typename Port>
result<> connector<Msgbuf, Port>::do_connect(monitor& mon)
{
    svc_->on_pre_connect(mon, port_);
    return port_.connect(addr_);
}


template <typename Msgbuf, typename Port>
Port connector<Msgbuf, Port>::make_port()
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
