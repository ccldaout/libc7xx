/*
 * c7event/service.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_SERVICE_HPP_LOADED__
#define C7_EVENT_SERVICE_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/monitor.hpp>
#include <c7socket.hpp>
#include <variant>


namespace c7 {
namespace event {


/*----------------------------------------------------------------------------
                                   I/O port
----------------------------------------------------------------------------*/

typedef std::variant<std::string, ::sockaddr_in> socket_addr;

class socket_port {
public:
    socket_port() = default;
    explicit socket_port(c7::socket&& sock);
    socket_port(const socket_port&) = delete;
    socket_port(socket_port&& o);
    socket_port& operator=(const socket_port&) = delete;
    socket_port& operator=(socket_port&&);

    // connector
    static socket_port tcp();
    static socket_port unix();

    // receiver, acceptor, connector
    int fd_number();

    // receiver
    bool is_alive();

    // receiver, acceptor
    template <typename Func> void add_on_close(Func&& func) {
	sock_.on_close += [f = std::forward<Func>(func)](c7::fd&){ f(); };
    }

    // connector
    result<> set_nonblocking(bool enable);

    // acceptor
    result<socket_port> accept();

    // connector
    result<> get_so_error(int *so_error);

    // connector
    result<> connect(const socket_addr& addr);

    // receiver
    void close();

    // [maybe] user defined service
    void set_different_endian();

    // multipart_msgbuf
    bool is_different_endian();

    // multipart_msgbuf
    io_result read_n(void *bufaddr, size_t req_n);

    // multipart_msgbuf
    io_result write_v(::iovec*& iov_io, int& ioc_io);

    // formattable
    void print(std::ostream& out, const std::string& spec) const;

private:
    c7::socket sock_;
    bool reverse_endian_ = false;
};


/*----------------------------------------------------------------------------
                         user side service interface
----------------------------------------------------------------------------*/

typedef std::variant<uint64_t, void*> provider_hint;

template <typename Msgbuf, typename Port = socket_port>
class service_interface:
    public std::enable_shared_from_this<service_interface<Msgbuf, Port>> {
public:
    typedef Msgbuf msgbuf_type;
    typedef Port port_type;

    service_interface() = default;
    virtual ~service_interface() = default;

    // interface for receiver
    virtual void on_attached(monitor&, Port&, provider_hint) {};
    virtual void on_message(monitor&, Port&, Msgbuf&) = 0;
    virtual void on_disconnected(monitor&, Port&, io_result&) = 0;
    virtual void on_error(monitor&, Port&, io_result&) = 0;
    virtual void on_detached(monitor&, Port&) {};
};


/*----------------------------------------------------------------------------
                                   receiver
----------------------------------------------------------------------------*/

template <typename Msgbuf, typename Port = socket_port>
class receiver: public provider_interface {
public:
    typedef std::shared_ptr<service_interface<Msgbuf, Port>> service_ptr;

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
	  typename Msgbuf = typename Service::msgbuf_type,
	  typename Port = typename Service::port_type>
auto make_receiver(Port&& port, std::shared_ptr<Service> svc, provider_hint hint = 0)
{
    return receiver<Msgbuf, Port>::make(std::move(port), std::move(svc), hint);
}

template <typename Service, typename Port>
result<> subscribe_receiver(Port&& port, std::shared_ptr<Service> svc, provider_hint hint = 0)
{
    return subscribe(make_receiver(std::move(port), std::move(svc), hint));
}

template <typename Service, typename Port>
result<> subscribe_receiver(monitor& mon, Port&& port, std::shared_ptr<Service> svc, provider_hint hint = 0)
{
    return mon.subscribe(make_receiver(std::move(port), std::move(svc), hint));
}


/*----------------------------------------------------------------------------
                                   acceptor
----------------------------------------------------------------------------*/

template <typename Msgbuf, typename Port = socket_port>
class acceptor: public provider_interface {
public:
    typedef std::shared_ptr<service_interface<Msgbuf, Port>> service_ptr;

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
	  typename Msgbuf = typename Service::msgbuf_type,
	  typename Port = typename Service::port_type>
auto make_acceptor(Port&& port, std::shared_ptr<Service> svc, provider_hint hint = 0)
{
    return acceptor<Msgbuf, Port>::make(std::move(port), std::move(svc), hint);
}

template <typename Service, typename Port>
result<> subscribe_acceptor(Port&& port, std::shared_ptr<Service> svc, provider_hint hint = 0)
{
    return subscribe(make_acceptor(std::move(port), std::move(svc), hint));
}

template <typename Service, typename Port>
result<> subscribe_acceptor(monitor& mon, Port&& port, std::shared_ptr<Service> svc, provider_hint hint = 0)
{
    return mon.subscribe(make_acceptor(std::move(port), std::move(svc), hint));
}


/*----------------------------------------------------------------------------
                                  connector
----------------------------------------------------------------------------*/

template <typename Msgbuf, typename Port = socket_port>
class connector: public provider_interface {
public:
    typedef std::shared_ptr<service_interface<Msgbuf, Port>> service_ptr;

    c7::delegate<void, Port&, const c7::result_base&> on_error;

    static auto make(const socket_addr& addr, service_ptr svc, provider_hint hint) {
	auto p = new connector(addr, std::move(svc), hint);
	return std::unique_ptr<connector>(p);
    }

    ~connector() override;
    int fd() override;
    uint32_t default_epoll_events() override;
    void on_subscribed(monitor& mon, int prvfd) override;
    void on_event(monitor& mon, int prvfd, uint32_t) override;

private:
    socket_addr addr_;
    service_ptr svc_;
    Port port_;
    c7::usec_t delay_s_ = 2;	// must be >= 2
    provider_hint hint_;

    connector(const socket_addr&, service_ptr&&, provider_hint);
    Port make_port();
    void start_timer(monitor& mon, int prvfd);
    void retry_connect(monitor& mon);
};

template <typename Service,
	  typename Msgbuf = typename Service::msgbuf_type,
	  typename Port = typename Service::port_type>
auto make_connector(const socket_addr& addr, std::shared_ptr<Service> svc, provider_hint hint = 0)
{
    return connector<Msgbuf, Port>::make(addr, svc, hint);
}

template <typename Service>
result<> subscribe_connector(const socket_addr& addr, std::shared_ptr<Service> svc, provider_hint hint = 0)
{
    return subscribe(make_connector(addr, svc, hint));
}


template <typename Service>
result<> subscribe_connector(monitor& mon,
			     const socket_addr& addr, std::shared_ptr<Service> svc, provider_hint hint = 0)
{
    return mon.subscribe(make_connector(addr, svc, hint));
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::event
} // namespace c7


#endif // c7event/service.hpp
