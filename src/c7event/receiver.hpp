/*
 * c7event/receiver.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_RECEIVER_HPP_LOADED__
#define C7_EVENT_RECEIVER_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/monitor.hpp>
#include <c7event/port.hpp>
#include <c7event/service.hpp>


namespace c7::event {


template <typename Msgbuf, typename Port = socket_port>
class receiver: public provider_interface {
public:
    using service_base = service_interface<Msgbuf, Port>;
    using service_ptr  = shared_service_ptr<Msgbuf, Port>;

    ~receiver() override {}
    int fd() override;
    void on_manage(monitor& mon, int prvfd) override;
    void on_event(monitor& mon, int prvfd, uint32_t events) override;
    void on_unmanage(monitor&, int prvfd) override;

    static auto make(Port&& port, service_ptr svc, provider_hint hint) {
	auto p = new receiver(std::move(port), std::move(svc), hint);
	return std::shared_ptr<receiver>(p);
    }

private:
    Port port_;
    service_ptr svc_;
    Msgbuf msgbuf_;
    provider_hint hint_;

    receiver(Port&& port, service_ptr&&, provider_hint hint);
};


template <typename Service>
auto make_receiver(typename Service::port_type&& port,
		   std::shared_ptr<Service> svc,
		   provider_hint hint = nullptr)
{
    using msgbuf_type = typename Service::msgbuf_type;
    using port_type   = typename Service::port_type;
    return receiver<msgbuf_type, port_type>::make(std::move(port), std::move(svc), hint);
}


template <typename Service>
result<> manage_receiver(typename Service::port_type&& port,
			 std::shared_ptr<Service> svc,
			 provider_hint hint = nullptr)
{
    return manage(make_receiver(std::move(port), std::move(svc), hint));
}


template <typename Service>
result<> manage_receiver(monitor& mon,
			 typename Service::port_type&& port,
			 std::shared_ptr<Service> svc,
			 provider_hint hint = nullptr)
{
    return mon.manage(make_receiver(std::move(port), std::move(svc), hint));
}


} // namespace c7::event


#endif // c7event/receiver.hpp
