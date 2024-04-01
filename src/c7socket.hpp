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
#ifndef C7_SOCKET_HPP_LOADED_
#define C7_SOCKET_HPP_LOADED_
#include <c7common.hpp>


#include <c7fd.hpp>
#include <netinet/in.h>
#include <sys/un.h>		// sockaddr_un
#include <utility>


namespace c7 {


struct sockaddr_gen {
    union {
	::sockaddr base;
	::sockaddr_in ipv4;
	::sockaddr_un unix;
    };
    sockaddr_gen(): base() {}
    explicit sockaddr_gen(const ::sockaddr_in& ipv4_): ipv4(ipv4_) {}
    explicit sockaddr_gen(const ::sockaddr_un& unix_): unix(unix_) {}
    void clear();
    void set_port(int port);
    bool is_ipv4() const;
    bool is_unix() const;
    ::socklen_t socklen() const;
    void print(std::ostream& out, const std::string&) const;
};


result<sockaddr_gen> sockaddr_unix(const std::string& path);
sockaddr_gen sockaddr_ipv4(uint32_t ipaddr, int port);
result<sockaddr_gen> sockaddr_ipv4(const std::string& host, int port);


class socket: public fd {
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
    static result<socket> unix();	// AF_UNIX, SOCK_STREAM
    static result<socket> unix_dg();	// AF_UNIX, SOCK_DGRAM

    result<> bind(const sockaddr_gen& addr);
    result<> bind(uint32_t ipaddr, int port);
    result<> bind(const std::string& host, int port);
    result<> bind(const std::string& path);		// UNIX domain

    result<> connect(const sockaddr_gen& addr);
    result<> connect(uint32_t ipaddr, int port);
    result<> connect(const std::string& host, int port);
    result<> connect(const std::string& path);		// UNIX domain

    result<> listen(int backlog = 0);

    result<socket> accept();

    result<sockaddr_gen> self() const;
    result<sockaddr_gen> peer() const;

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

    io_result recvfrom(void *buf, size_t bufsize, sockaddr_gen&, int flags = 0);
    template <typename T>
    io_result recvfrom(T *buf, size_t bufsize, sockaddr_gen& addr, int flags = 0) {
	return recvfrom(static_cast<void *>(buf), bufsize, addr, flags);
    }
    template <typename T>
    io_result recvfrom(T *buf, sockaddr_gen& addr, int flags = 0) {
	return recvfrom(buf, sizeof(buf), addr, flags);
    }

    io_result sendto(const void *buf, size_t bufsize, const sockaddr_gen&, int flags = 0);
    template <typename T>
    io_result sendto(const T *buf, size_t bufsize, const sockaddr_gen& addr, int flags = 0) {
	return sendto(static_cast<const void*>(buf), bufsize, addr, flags);
    }
    template <typename T>
    io_result sendto(const T *buf, const sockaddr_gen& addr, int flags = 0) {
	return sendto(buf, sizeof(*buf), addr, flags);
    }

    std::string to_string(const std::string& spec) const override;
    void print(std::ostream& out, const std::string& spec) const override;

protected:
    void setup_str() const;
};

result<std::pair<socket, socket>> socketpair(bool stream = true);

result<socket> tcp_server(const sockaddr_gen& inaddr, int rcvbuf_size = 0, int backlog = 0);
result<socket> tcp_server(uint32_t ipaddr, int port, int rcvbuf_size = 0, int backlog = 0);
result<socket> tcp_server(const std::string& host, int port, int rcvbuf_size = 0, int backlog = 0);
result<socket> tcp_client(const sockaddr_gen& inaddr, int rcvbuf_size = 0);
result<socket> tcp_client(uint32_t ipaddr, int port, int rcvbuf_size = 0);
result<socket> tcp_client(const std::string& host, int port, int rcvbuf_size = 0);

result<socket> udp_binded(const sockaddr_gen& inaddr);
result<socket> udp_binded(uint32_t ipaddr, int port);
result<socket> udp_binded(const std::string& host, int port);

result<socket> unix_server(const std::string& addr, int backlog = 0);
result<socket> unix_client(const std::string& addr);

result<socket> unix_dg_binded(const std::string& addr);
result<socket> unix_dg_binded();


} // namespace c7


#endif // c7socket.hpp
