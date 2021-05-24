/*
 * c7event/acceptor_impl.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_ACCEPTOR_IMPL_HPP_LOADED__
#define C7_EVENT_ACCEPTOR_IMPL_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/acceptor.hpp>
#include <c7event/receiver_impl.hpp>


namespace c7::event {


// implementation of acceptor


template <typename Msgbuf, typename Port, typename Receiver>
acceptor<Msgbuf, Port, Receiver>::acceptor(Port&& port, service_ptr&& svc, provider_hint hint):
    provider_interface(), port_(std::move(port)), svc_(std::move(svc)), hint_(hint)
{
}


template <typename Msgbuf, typename Port, typename Receiver>
int acceptor<Msgbuf, Port, Receiver>::fd()
{
    return port_.fd_number();
}


template <typename Msgbuf, typename Port, typename Receiver>
void acceptor<Msgbuf, Port, Receiver>::on_subscribed(monitor& mon, int prvfd)
{
    port_.add_on_close([&mon, prvfd](){ mon.unsubscribe(prvfd); });
}


template <typename Msgbuf, typename Port, typename Receiver>
void acceptor<Msgbuf, Port, Receiver>::on_event(monitor& mon, int, uint32_t)
{
    if (auto res = port_.accept(); !res) {
	on_error(port_, res);
    } else {
	auto port = std::move(res.value());
	auto prv = make_receiver<service_interface<Msgbuf, Port>, Receiver>(std::move(port), svc_, hint_);
	mon.subscribe(std::move(prv));
    }
}


} // namespace c7::event


#endif // c7event/acceptor_impl.hpp
