/*
 * c7event/connector.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_CONNECTOR_HPP_LOADED_
#define C7_EVENT_CONNECTOR_HPP_LOADED_
#include <c7common.hpp>


#include <c7event/monitor.hpp>
#include <c7event/port.hpp>
#include <c7event/receiver.hpp>
#include <c7event/service.hpp>


namespace c7::event {


template <typename Msgbuf, typename Port = socket_port>
class connector: public provider_interface {
public:
    using service_base = service_interface<Msgbuf, Port>;
    using service_ptr  = shared_service_ptr<Msgbuf, Port>;

    c7::delegate<void, Port&, const c7::result_base&> on_error;

    static auto make(const sockaddr_gen& addr, service_ptr svc, provider_hint hint) {
	auto p = new connector(addr, std::move(svc), hint);
	return std::shared_ptr<connector>(p);
    }

    ~connector() override;
    int fd() override;
    uint32_t default_epoll_events() override;
    void on_manage(monitor& mon, int prvfd) override;
    void on_event(monitor& mon, int prvfd, uint32_t) override;

private:
    sockaddr_gen addr_;
    service_ptr svc_;
    Port port_;
    c7::usec_t delay_s_ = 2;	// must be >= 2
    provider_hint hint_;

    connector(const sockaddr_gen&, service_ptr&&, provider_hint);
    void start_timer(monitor& mon, int prvfd);
    void retry_connect(monitor& mon);
    result<> do_connect(monitor& mon);
    Port make_port();
};


template <typename Service>
auto make_connector(const sockaddr_gen& addr,
		    std::shared_ptr<Service> svc,
		    provider_hint hint = nullptr)
{
    using msgbuf_type = typename Service::msgbuf_type;
    using port_type   = typename Service::port_type;
    return connector<msgbuf_type, port_type>::make(addr, svc, hint);
}


template <typename Service>
result<> manage_connector(const sockaddr_gen& addr,
			  std::shared_ptr<Service> svc,
			  provider_hint hint = nullptr)
{
    return manage(make_connector(addr, svc, hint));
}


template <typename Service>
result<> manage_connector(monitor& mon,
			  const sockaddr_gen& addr,
			  std::shared_ptr<Service> svc,
			  provider_hint hint = nullptr)
{
    return mon.manage(make_connector(addr, svc, hint));
}


} // namespace c7::event


#endif // c7event/connector.hpp
