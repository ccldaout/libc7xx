/*
 * c7event/acceptor.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_ACCEPTOR_HPP_LOADED_
#define C7_EVENT_ACCEPTOR_HPP_LOADED_
#include <c7common.hpp>


#include <c7event/port.hpp>
#include <c7event/monitor.hpp>
#include <c7event/receiver.hpp>
#include <c7event/service.hpp>


namespace c7::event {



template <typename Service>
using service_factory = std::function<std::shared_ptr<Service>()>;


template <typename Msgbuf, typename Port = socket_port>
class acceptor: public provider_interface {
private:
    using service_base = service_interface<Msgbuf, Port>;
    using service_ptr  = shared_service_ptr<Msgbuf, Port>;
    using service_make = service_factory<service_base>;

public:
    c7::delegate<void, Port&, c7::result_base&> on_error;

    // create service object for each connection
    template <typename ServiceFactory,
	      typename = typename std::invoke_result_t<ServiceFactory>::element_type>
    static auto make(Port&& port, ServiceFactory&& svc_factory, provider_hint hint) {
	service_make wrap_factory =
	    [factory=std::forward<ServiceFactory>(svc_factory)]() mutable -> service_ptr {
		return factory();
	    };
	auto p = new acceptor(std::move(port), std::move(wrap_factory), hint);
	return std::shared_ptr<acceptor>(p);
    }

    // share service object for all connection
    static auto make(Port&& port, service_ptr svc, provider_hint hint) {
	return make(std::move(port), [svc](){ return svc; }, hint);
    }

    ~acceptor() override {}
    int fd() override;
    void on_manage(monitor& mon, int prvfd) override;
    void on_event(monitor& mon, int prvfd, uint32_t events) override;

private:
    Port port_;
    service_make svc_factory_;
    provider_hint hint_;

    acceptor(Port&& port, service_make svc_factory, provider_hint hint);
};


// create service object for each connection

template <typename ServiceFactory,
	  typename Service = typename std::invoke_result_t<ServiceFactory>::element_type>
auto make_acceptor(typename Service::port_type&& port,
		   ServiceFactory&& svc_factory,
		   provider_hint hint = nullptr)
{
    using msgbuf_type = typename Service::msgbuf_type;
    using port_type   = typename Service::port_type;
    return acceptor<msgbuf_type, port_type>::make(std::move(port),
						  std::forward<ServiceFactory>(svc_factory),
						  hint);
}


template <typename ServiceFactory,
	  typename Service = typename std::invoke_result_t<ServiceFactory>::element_type>
auto manage_acceptor(typename Service::port_type&& port,
		     ServiceFactory&& svc_factory,
		     provider_hint hint = nullptr)
{
    return manage(make_acceptor(std::move(port),
				std::forward<ServiceFactory>(svc_factory),
				hint));
}


template <typename ServiceFactory,
	  typename Service = typename std::invoke_result_t<ServiceFactory>::element_type>
auto manage_acceptor(monitor& mon,
		     typename Service::port_type&& port,
		     ServiceFactory&& svc_factory,
		     provider_hint hint = nullptr)
{
    return mon.manage(make_acceptor(std::move(port),
				    std::forward<ServiceFactory>(svc_factory),
				    hint));
}


// share service object for all connection

template <typename Service>
auto make_acceptor(typename Service::port_type&& port,
		   std::shared_ptr<Service> svc,
		   provider_hint hint = nullptr)
{
    using msgbuf_type = typename Service::msgbuf_type;
    using port_type   = typename Service::port_type;
    return acceptor<msgbuf_type, port_type>::make(std::move(port), std::move(svc), hint);
}


template <typename Service>
result<> manage_acceptor(typename Service::port_type&& port,
			 std::shared_ptr<Service> svc,
			 provider_hint hint = nullptr)
{
    return manage(make_acceptor(std::move(port), std::move(svc), hint));
}


template <typename Service>
result<> manage_acceptor(monitor& mon,
			 typename Service::port_type&& port,
			 std::shared_ptr<Service> svc,
			 provider_hint hint = nullptr)
{
    return mon.manage(make_acceptor(std::move(port), std::move(svc), hint));
}


} // namespace c7::event


#endif // c7event/acceptor.hpp
