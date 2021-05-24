/*
 * c7event/acceptor.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_ACCEPTOR_HPP_LOADED__
#define C7_EVENT_ACCEPTOR_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/port.hpp>
#include <c7event/monitor.hpp>
#include <c7event/receiver.hpp>
#include <c7event/service.hpp>


namespace c7::event {


template <typename Msgbuf, typename Port = socket_port,
	  typename Receiver = receiver<Msgbuf, Port>>
class acceptor: public provider_interface {
public:
    typedef shared_service_ptr<Msgbuf, Port> service_ptr;

    c7::delegate<void, Port&, c7::result_base&> on_error;

    static auto make(Port&& port, service_ptr svc, provider_hint hint) {
	auto p = new acceptor(std::move(port), std::move(svc), hint);
	return std::unique_ptr<acceptor>(p);
    }

    ~acceptor() override {}
    int fd() override;
    void on_subscribed(monitor& mon, int prvfd) override;
    void on_event(monitor& mon, int prvfd, uint32_t events) override;

private:
    Port port_;
    service_ptr svc_;
    provider_hint hint_;

    acceptor(Port&& port, service_ptr&& svc, provider_hint hint);
};


template <typename Service,
	  typename Receiver = receiver<typename Service::msgbuf_type,
				       typename Service::port_type>>
auto make_acceptor(typename Service::port_type&& port,
		   std::shared_ptr<Service> svc,
		   provider_hint hint = nullptr)
{
    typedef typename Service::msgbuf_type msgbuf_type;
    typedef typename Service::port_type port_type;
    return acceptor<msgbuf_type, port_type, Receiver>::make(std::move(port), std::move(svc), hint);
}


template <typename Service,
	  typename Receiver = receiver<typename Service::msgbuf_type,
				       typename Service::port_type>>
result<> subscribe_acceptor(typename Service::port_type&& port,
			    std::shared_ptr<Service> svc,
			    provider_hint hint = nullptr)
{
    return subscribe(make_acceptor<Service, Receiver>(std::move(port), std::move(svc), hint));
}


template <typename Service,
	  typename Receiver = receiver<typename Service::msgbuf_type,
				       typename Service::port_type>>
result<> subscribe_acceptor(monitor& mon,
			    typename Service::port_type&& port,
			    std::shared_ptr<Service> svc,
			    provider_hint hint = nullptr)
{
    return mon.subscribe(make_acceptor<Service, Receiver>(std::move(port), std::move(svc), hint));
}


} // namespace c7::event


#endif // c7event/acceptor.hpp
