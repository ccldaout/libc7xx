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


template <typename Msgbuf, typename Port>
acceptor<Msgbuf, Port>::acceptor(Port&& port, service_make svc_factory, provider_hint hint):
    provider_interface(), port_(std::move(port)), svc_factory_(std::move(svc_factory)), hint_(hint)
{
}


template <typename Msgbuf, typename Port>
int acceptor<Msgbuf, Port>::fd()
{
    return port_.fd_number();
}


template <typename Msgbuf, typename Port>
void acceptor<Msgbuf, Port>::on_manage(monitor& mon, int prvfd)
{
    port_.add_on_close([&mon, prvfd](){ mon.unmanage(prvfd); });
}


template <typename Msgbuf, typename Port>
void acceptor<Msgbuf, Port>::on_event(monitor& mon, int, uint32_t)
{
    if (auto res = port_.accept(); !res) {
	on_error(port_, res);
    } else {
	auto port = std::move(res.value());
	auto prv = make_receiver(std::move(port), svc_factory_(), hint_);
	mon.manage(std::move(prv));
    }
}


} // namespace c7::event


#endif // c7event/acceptor_impl.hpp
