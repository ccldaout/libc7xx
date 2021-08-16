/*
 * c7event/receiver_impl.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_RECEIVER_IMPL_HPP_LOADED__
#define C7_EVENT_RECEIVER_IMPL_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/receiver.hpp>


namespace c7::event {


// implementation of receiver


template <typename Msgbuf, typename Port>
receiver<Msgbuf, Port>::receiver(Port&& port, service_ptr&& svc, provider_hint hint):
    provider_interface(), port_(std::move(port)), svc_(std::move(svc)), msgbuf_(), hint_(hint)
{
}


template <typename Msgbuf, typename Port>
int receiver<Msgbuf, Port>::fd()
{
    return port_.fd_number();
}


template <typename Msgbuf, typename Port>
void receiver<Msgbuf, Port>::on_manage(monitor& mon, int prvfd)
{
    port_.add_on_close([&mon, prvfd](){ mon.unmanage(prvfd); });
    svc_->on_attached(mon, port_, hint_);
}


template <typename Msgbuf, typename Port>
void receiver<Msgbuf, Port>::on_event(monitor& mon, int, uint32_t)
{
    auto io_res = msgbuf_.recv(port_);
    if (io_res.get_status() == io_result::status::OK) {
	svc_->on_message(mon, port_, msgbuf_);
    } else if (io_res.get_status() == io_result::status::CLOSED) {
	svc_->on_disconnected(mon, port_, io_res);
	if (port_.is_alive()) {
	    port_.close();
	}
    } else {
	svc_->on_error(mon, port_, io_res);
	if (port_.is_alive()) {
	    port_.close();
	}
    }
}


template <typename Msgbuf, typename Port>
void receiver<Msgbuf, Port>::on_unmanage(monitor& mon, int)
{
    svc_->on_detached(mon, port_, hint_);
}


} // namespace c7::event


#endif // c7event/receiver_impl.hpp
