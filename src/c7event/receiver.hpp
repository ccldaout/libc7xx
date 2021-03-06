/*
 * c7event/receiver.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
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
    typedef service_interface<Msgbuf, Port> service_base;
    typedef shared_service_ptr<Msgbuf, Port> service_ptr;

    ~receiver() override {}
    int fd() override;
    void on_subscribed(monitor& mon, int prvfd) override;
    void on_event(monitor& mon, int prvfd, uint32_t events) override;
    void on_unsubscribed(monitor&, int prvfd) override;

    static auto make(Port&& port, service_ptr svc, provider_hint hint) {
	auto p = new receiver(std::move(port), std::move(svc), hint);
	return std::unique_ptr<receiver>(p);
    }

private:
    Port port_;
    service_ptr svc_;
    Msgbuf msgbuf_;
    provider_hint hint_;

    receiver(Port&& port, service_ptr&&, provider_hint hint);
};


template <typename Service,
	  typename Receiver = receiver<typename Service::msgbuf_type,
				       typename Service::port_type>>
auto make_receiver(typename Service::port_type&& port,
		   std::shared_ptr<Service> svc,
		   provider_hint hint = nullptr)
{
    return Receiver::make(std::move(port), std::move(svc), hint);
}


template <typename Service,
	  typename Receiver = receiver<typename Service::msgbuf_type,
				       typename Service::port_type>>
result<> subscribe_receiver(typename Service::port_type&& port,
			    std::shared_ptr<Service> svc,
			    provider_hint hint = nullptr)
{
    return subscribe(make_receiver<Service, Receiver>(std::move(port), std::move(svc), hint));
}


template <typename Service,
	  typename Receiver = receiver<typename Service::msgbuf_type,
				       typename Service::port_type>>
result<> subscribe_receiver(monitor& mon,
			    typename Service::port_type&& port,
			    std::shared_ptr<Service> svc,
			    provider_hint hint = nullptr)
{
    return mon.subscribe(make_receiver<Service, Receiver>(std::move(port), std::move(svc), hint));
}


} // namespace c7::event


#endif // c7event/receiver.hpp
