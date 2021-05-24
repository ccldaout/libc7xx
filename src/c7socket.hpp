/*
 * c7socket.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=448649916
 */
#ifndef C7_SOCKET_HPP_LOADED__
#define C7_SOCKET_HPP_LOADED__
#include <c7common.hpp>


#include <c7fd.hpp>
#include <netinet/in.h>
#include <sys/un.h>		// sockaddr_un
#include <utility>


void print_type(std::ostream& out, const std::string&, const ::sockaddr_in&);


namespace c7 {


result<std::pair<::sockaddr_un, size_t>> sockaddr_unix(const std::string& path);
::sockaddr_in sockaddr_ipv4(uint32_t ipaddr, int port);
result<::sockaddr_in> sockaddr_ipv4(const std::string& host, int port);
void sockaddr_ipv4_port(::sockaddr_in& inaddr, int port);


class socket: public fd {
private:
    void setup_ipv4_str() const;
    void setup_unix_str() const;

protected:
    void setup_str() const;

public:
    socket(const socket&) = delete;
    socket& operator=(const socket&) = delete;

    socket(): fd() {}
    explicit socket(int fdnum): fd(fdnum) {}
    socket(socket&& o): fd(std::move(o)) {}
    socket(fd&& o): fd(std::move(o)) {}

    socket& operator=(socket&& o) {
	fd::operator=(std::move(o));
	return *this;
    }

    socket& operator=(fd&& o) {
	fd::operator=(std::move(o));
	return *this;
    }

    static result<socket> make_socket(int domain, int type, int protocol = 0);
    static result<socket> tcp();	// AF_INET, SOCK_STREAM
    static result<socket> udp();	// AF_INET, SOCK_DGRAM
    static result<socket> unix();	// AF_UNXI, SOCK_STREAM

    result<> bind(const ::sockaddr_in& inaddr);
    result<> bind(uint32_t ipaddr, int port);
    result<> bind(const std::string& host, int port);
    result<> bind(const std::string& path);		// UNIX domain

    result<> connect(const ::sockaddr_in& inaddr);
    result<> connect(uint32_t ipaddr, int port);
    result<> connect(const std::string& host, int port);
    result<> connect(const std::string& path);	// UNIX domain

    result<> listen(int backlog = 0);
    
    result<socket> accept();
    
    result<::sockaddr_in> self_ipv4() const;
    result<::sockaddr_in> peer_ipv4() const;

    result<> getsockopt(int level, int optname, void *optval, socklen_t *optlen) const;
    result<> setsockopt(int level, int optname, const void *optval, socklen_t optlen);

    result<> tcp_keepalive(bool enable);
    result<> tcp_nodelay(bool enable);
    result<> set_rcvbuf(int nbytes);	// server:before listen, client:before conenct
    result<> set_sndbuf(int nbytes);
    result<> set_sndtmo(c7::usec_t timeout);
    result<> set_rcvtmo(c7::usec_t timeout);

    result<> shutdown_r();
    result<> shutdown_w();
    result<> shutdown_rw();

    std::string to_string(const std::string& spec) const override;
    void print(std::ostream& out, const std::string& spec) const override;
};

result<std::pair<socket, socket>> socketpair(bool stream = true);

result<socket> tcp_server(const ::sockaddr_in& inaddr, int rcvbuf_size = 0, int backlog = 0);
result<socket> tcp_server(uint32_t ipaddr, int port, int rcvbuf_size = 0, int backlog = 0);
result<socket> tcp_server(const std::string& host, int port, int rcvbuf_size = 0, int backlog = 0);
result<socket> tcp_client(const ::sockaddr_in& inaddr, int rcvbuf_size = 0);
result<socket> tcp_client(uint32_t ipaddr, int port, int rcvbuf_size = 0);
result<socket> tcp_client(const std::string& host, int port, int rcvbuf_size = 0);

result<socket> udp_server(const ::sockaddr_in& inaddr);
result<socket> udp_server(uint32_t ipaddr, int port);
result<socket> udp_server(const std::string& host, int port);
result<socket> udp_client(const ::sockaddr_in& inaddr);
result<socket> udp_client(uint32_t ipaddr, int port);
result<socket> udp_client(const std::string& host, int port);

result<socket> unix_server(const std::string& addr, int backlog = 0);
result<socket> unix_client(const std::string& addr);


} // namespace c7


#endif // c7socket.hpp
