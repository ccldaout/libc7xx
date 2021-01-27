/*
 * c7socket.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7socket.hpp>
#include <unistd.h>
#include <cstring>
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/socket.h>


namespace c7 {


/*----------------------------------------------------------------------------
                                socket address
----------------------------------------------------------------------------*/

result<std::pair<::sockaddr_un, size_t>> sockaddr_unix(const std::string& path)
{
    ::sockaddr_un unaddr;
    (void)std::memset(&unaddr, 0, sizeof(unaddr));
    if (sizeof(unaddr.sun_path) + 1 < path.size()) {
	return c7result_err(EINVAL, "sockaddr_un: too long path: %{}", path);
    }
    (void)std::strcpy(unaddr.sun_path, path.c_str());
    unaddr.sun_family = AF_UNIX;
    size_t addrlen = SUN_LEN(&unaddr);
    return c7result_ok(std::make_pair(unaddr, addrlen));
}

::sockaddr_in sockaddr_ipv4(uint32_t ipaddr, int port)
{
    ::sockaddr_in inaddr;
    (void)std::memset(&inaddr, 0, sizeof(inaddr));
    inaddr.sin_addr.s_addr = ipaddr;
    inaddr.sin_family = AF_INET;
    inaddr.sin_port   = htons(port);
    return inaddr;
}

void sockaddr_ipv4_port(::sockaddr_in& inaddr, int port)
{
    inaddr.sin_port = htons(port);
}

result<::sockaddr_in> sockaddr_ipv4(const std::string& host, int port)
{
    uint32_t ipaddr;

    if (host.empty() || host == "" || host == "*") {
	ipaddr = htonl(INADDR_ANY);
    } else {
	::addrinfo hints = { .ai_family = AF_INET };
	::addrinfo *result;
	int err = ::getaddrinfo(host.c_str(), nullptr, &hints, &result);
	if (err != C7_SYSOK) {
	    return c7result_err(EINVAL,
				"getaddrinfo(%{}) failed: %{}", host, gai_strerror(err));
	}
	ipaddr = ((::sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
	::freeaddrinfo(result);
    }
    return c7result_ok(sockaddr_ipv4(ipaddr, port));
}


/*----------------------------------------------------------------------------
                                    socket
----------------------------------------------------------------------------*/

void socket::setup_ipv4_str(const std::string& state)
{
    auto res1 = self_ipv4();
    auto res2 = peer_ipv4();
    if (!res1) {
	str_ = c7::format("%{}<%{},?,%{}>", name_, fdnum_, state);
	return;
    }
    auto& self = res1.value();
    if (res2) {
	auto& peer = res2.value();
	const uint8_t *ip = (const uint8_t *)&peer.sin_addr.s_addr;
	str_ = c7::format("%{}<%{},peer:%{}.%{}.%{}.%{}:%{},self:%{},%{}>",
			  name_, fdnum_,
			  uint32_t(ip[0]), uint32_t(ip[1]), uint32_t(ip[2]), uint32_t(ip[3]),
			  ntohs(peer.sin_port),
			  ntohs(self.sin_port),
			  state);
    } else {
	const uint8_t *ip = (const uint8_t *)&self.sin_addr.s_addr;
	str_ = c7::format("%{}<%{},self:%{}.%{}.%{}.%{}:%{},%{}>",
			  name_, fdnum_,
			  uint32_t(ip[0]), uint32_t(ip[1]), uint32_t(ip[2]), uint32_t(ip[3]),
			  ntohs(self.sin_port),
			  state);
    }
}

void socket::setup_unix_str(const std::string& state)
{
    ::sockaddr_un unaddr;
    socklen_t n = sizeof(unaddr);
    int ret = ::getsockname(fdnum_, (::sockaddr *)&unaddr, &n);
    if (ret == C7_SYSERR) {
	str_ = c7::format("%{}<%{},?,%{}>", name_, fdnum_, state);
    } else {
	str_ = c7::format("%{}<%{},%{},%{}>", name_, fdnum_, unaddr.sun_path, state);
    }
}

void socket::setup_str(const std::string& state)
{
    ::sockaddr addr;
    socklen_t n = sizeof(addr);
    int ret = ::getsockname(fdnum_, &addr, &n);
    if (ret == C7_SYSOK && addr.sa_family == AF_INET) {
	setup_ipv4_str(state);
    } else if (ret == C7_SYSOK && addr.sa_family == AF_UNIX) {
	setup_unix_str(state);
    } else {
	str_ = c7::format("%{}<%{},?,%{}>", name_, fdnum_, state);
    }
}

result<socket> socket::make_socket(int domain, int type, int protocol,
				   const std::string& name)
{
    int fd = ::socket(domain, type, protocol);
    if (fd == C7_SYSERR) {
	return c7result_err(errno, "socket(%{}, %{}, %{}) failed", domain, type, protocol);
    }
    auto sock = c7::socket(fd, name);
    sock.setup_str("!");
    return c7result_ok(std::move(sock));
}

result<socket> socket::tcp()
{
    return socket::make_socket(AF_INET, SOCK_STREAM, 0, "tcp");
}

result<socket> socket::udp()
{
    return socket::make_socket(AF_INET, SOCK_DGRAM, 0, "udp");
}

result<socket> socket::unix()
{
    return socket::make_socket(AF_UNIX, SOCK_STREAM, 0, "un");
}

result<> socket::bind(const ::sockaddr_in& inaddr)
{
    int reuse = 1;
    (void)::setsockopt(fdnum_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (::bind(fdnum_, (::sockaddr *)&inaddr, sizeof(inaddr)) == C7_SYSERR) {
	return c7result_err(errno,
			    "bind(%{}, %{}) failed", *this, inaddr);
    }
    setup_str("B");
    return c7result_ok();
}

result<> socket::bind(uint32_t ipaddr, int port)
{
    return socket::bind(sockaddr_ipv4(ipaddr, port));
}

result<> socket::bind(const std::string& host, int port)
{
    auto res = sockaddr_ipv4(host, port);
    if (!res) {
	return c7result_err(std::move(res),
			    "bind(%{}, %{}, %{}) failed", fdnum_, host, port);
    }
    return socket::bind(res.value());

}

result<> socket::bind(const std::string& path)	// UNIX domain
{
    auto res = sockaddr_unix(path);
    if (!res) {
	return c7result_err(std::move(res), "bind(%{}, %{}) failed", fdnum_, path);
    }
    auto [unaddr, addrlen] = res.value();
    if (::bind(fdnum_, (struct sockaddr *)&unaddr, addrlen) == C7_SYSERR) {
	return c7result_err(errno, "bind(%{}, %{}) failed", fdnum_, path);
    }
    setup_str("B");
    return c7result_ok();
}

result<> socket::connect(const ::sockaddr_in& inaddr)
{
    if (::connect(fdnum_, (::sockaddr *)&inaddr, sizeof(inaddr)) == C7_SYSERR) {
	return c7result_err(errno,
			    "connect(%{}, %{}) failed", fdnum_, inaddr);
    }
    (void)tcp_keepalive(true);
    setup_str("C");
    return c7result_ok();
}

result<> socket::connect(uint32_t ipaddr, int port)
{
    return socket::connect(sockaddr_ipv4(ipaddr, port));
}

result<> socket::connect(const std::string& host, int port)
{
    auto res = sockaddr_ipv4(host, port);
    if (!res) {
	return c7result_err(std::move(res),
			    "connect(%{}, %{}, %{}) failed", fdnum_, host, port);
    }
    return socket::connect(res.value());
}

result<> socket::connect(const std::string& path)	// UNIX domain
{
    auto res = sockaddr_unix(path);
    if (!res) {
	return c7result_err(std::move(res), "connect(%{}, %{}) failed", fdnum_, path);
    }
    auto [unaddr, addrlen] = res.value();
    if (::connect(fdnum_, (struct sockaddr *)&unaddr, addrlen) == C7_SYSERR) {
	return c7result_err(errno, "connect(%{}, %{}) failed", fdnum_, path);
    }
    setup_str("C");
    return c7result_ok();
}

result<> socket::listen(int backlog)
{
    if (::listen(fdnum_, backlog) == C7_SYSERR) {
	return c7result_err(errno, "listen(%{}, %{}) failed", fdnum_, backlog);
    }
    setup_str("S");
    return c7result_ok();
}
    
result<socket> socket::accept()
{
    int newfd = ::accept(fdnum_, nullptr, nullptr);
    if (newfd == C7_SYSERR) {
	return c7result_err(errno, "accept(%{}) failed", fdnum_);
    }
    auto newsock = c7::socket(newfd);
    newsock.setup_str("A");
    (void)newsock.tcp_keepalive(true);
    return c7result_ok(std::move(newsock));
}
    
result<::sockaddr_in> socket::self_ipv4()
{
    ::sockaddr_in inaddr;
    socklen_t n = sizeof(inaddr);
    int ret = ::getsockname(fdnum_, (::sockaddr *)&inaddr, &n);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "getsockname(%{}) [IPv4] failed", fdnum_);
    }
    return c7result_ok(inaddr);
}

result<::sockaddr_in> socket::peer_ipv4()
{
    ::sockaddr_in inaddr;
    socklen_t n = sizeof(inaddr);
    int ret = ::getpeername(fdnum_, (struct sockaddr *)&inaddr, &n);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "setpeername(%{}) [IPv4] failed", fdnum_);
    }
    return c7result_ok(inaddr);
}

result<> socket::getsockopt(int level, int optname, void *optval, socklen_t *optlen)
{
    int ret = ::getsockopt(fdnum_, level, optname, optval, optlen);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "getsockopt(%{}, %{}, %{}) failed",
			    fdnum_, level, optname);
    }
    return c7result_ok();
}

result<> socket::setsockopt(int level, int optname, const void *optval, socklen_t optlen)
{
    int ret = ::setsockopt(fdnum_, level, optname, optval, optlen);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "setsockopt(%{}, %{}, %{}) failed",
			    fdnum_, level, optname);
    }
    return c7result_ok();
}

result<> socket::tcp_keepalive(bool enable)
{
    int avail = int(enable);
    return socket::setsockopt(SOL_SOCKET, SO_KEEPALIVE, &avail, sizeof(avail));
}

result<> socket::tcp_nodelay(bool enable)
{
    int avail = int(enable);
    return socket::setsockopt(IPPROTO_TCP, TCP_NODELAY, &avail, sizeof(avail));
}

result<> socket::set_rcvbuf(int nbytes)	// server:before listen, client:before conenct
{
    return socket::setsockopt(SOL_SOCKET, SO_RCVBUF, &nbytes, sizeof(nbytes));
}
    
result<> socket::set_sndbuf(int nbytes)
{
    return socket::setsockopt(SOL_SOCKET, SO_SNDBUF, &nbytes, sizeof(nbytes));
}

result<> socket::set_sndtmo(c7::usec_t timeout)
{
    ::timeval tmout;
    tmout.tv_sec  = timeout / C7_TIME_S_us;
    tmout.tv_usec = timeout % C7_TIME_S_us;
    return socket::setsockopt(SOL_SOCKET, SO_SNDTIMEO, &tmout, sizeof(tmout));
}

result<> socket::set_rcvtmo(c7::usec_t timeout)
{
    ::timeval tmout;
    tmout.tv_sec  = timeout / C7_TIME_S_us;
    tmout.tv_usec = timeout % C7_TIME_S_us;
    return socket::setsockopt(SOL_SOCKET, SO_RCVTIMEO, &tmout, sizeof(tmout));
}

result<> socket::shutdown_r()
{
    if (::shutdown(fdnum_, SHUT_RD) == C7_SYSERR) {
	return c7result_err(errno, "shutdown(%{}, RD) failed", fdnum_);
    }
    return c7result_ok();
}

result<> socket::shutdown_w()
{
    if (::shutdown(fdnum_, SHUT_WR) == C7_SYSERR) {
	return c7result_err(errno, "shutdown(%{}, WR) failed", fdnum_);
    }
    return c7result_ok();
}

result<> socket::shutdown_rw()
{
    if (::shutdown(fdnum_, SHUT_RDWR) == C7_SYSERR) {
	return c7result_err(errno, "shutdown(%{}, RDWR) failed", fdnum_);
    }
    return c7result_ok();
}

const std::string& socket::format(const std::string spec) const
{
    return str_;
}


/*----------------------------------------------------------------------------
                                  non-member
----------------------------------------------------------------------------*/

result<std::pair<socket, socket>> socketpair(bool stream)
{
    int fdv[2];
    int sock_type = stream ? SOCK_STREAM : SOCK_DGRAM;
    if (::socketpair(AF_UNIX, sock_type, 0, fdv) == C7_SYSERR) {
	return c7result_err(errno, "socketpair(%{}, %{}) failed", AF_UNIX, sock_type);
    }
    return c7result_ok(std::make_pair(c7::socket(fdv[0]),
				      c7::socket(fdv[1])));
}

result<socket> tcp_server(const ::sockaddr_in& inaddr, int rcvbuf_size, int backlog)
{
    auto res = socket::tcp();
    if (!res) {
	return res;
    }

    auto sock = std::move(res.value());
    auto defer = c7::defer([&sock](){ sock.close(); });

    if (auto res = sock.bind(inaddr); !res) {
	return std::move(res);
    }
    if (rcvbuf_size > 0) {
	if (auto res = sock.set_rcvbuf(rcvbuf_size); !res) {
	    return std::move(res);
	}
    }
    if (auto res = sock.listen(backlog); !res) {
	return std::move(res);
    }

    defer.cancel();
    return c7result_ok(std::move(sock));
}

result<socket> tcp_server(uint32_t ipaddr, int port, int rcvbuf_size, int backlog)
{
    return tcp_server(sockaddr_ipv4(ipaddr, port), rcvbuf_size, backlog);
}

result<socket> tcp_server(const std::string& host, int port, int rcvbuf_size, int backlog)
{
    auto res = sockaddr_ipv4(host, port);
    if (!res) {
	return std::move(res);
    }
    return tcp_server(res.value(), rcvbuf_size, backlog);
}

result<socket> tcp_client(const ::sockaddr_in& inaddr, int rcvbuf_size)
{
    auto res = socket::tcp();
    if (!res) {
	return res;
    }

    auto sock = std::move(res.value());
    auto defer = c7::defer([&sock](){ sock.close(); });

    if (rcvbuf_size > 0) {
	if (auto res = sock.set_rcvbuf(rcvbuf_size); !res) {
	    return std::move(res);
	}
    }
    if (auto res = sock.connect(inaddr); !res) {
	return std::move(res);
    }

    defer.cancel();
    return c7result_ok(std::move(sock));
}

result<socket> tcp_client(uint32_t ipaddr, int port, int rcvbuf_size)
{
    return tcp_client(sockaddr_ipv4(ipaddr, port), rcvbuf_size);
}

result<socket> tcp_client(const std::string& host, int port, int rcvbuf_size)
{
    auto res = sockaddr_ipv4(host, port);
    if (!res) {
	return std::move(res);
    }
    return tcp_client(res.value(), rcvbuf_size);
}

result<socket> udp_server(const ::sockaddr_in& inaddr)
{
    auto res = socket::udp();
    if (!res) {
	return res;
    }

    auto sock = std::move(res.value());
    auto defer = c7::defer([&sock](){ sock.close(); });

    if (auto res = sock.bind(inaddr); !res) {
	return std::move(res);
    }

    defer.cancel();
    return c7result_ok(std::move(sock));
}

result<socket> udp_server(uint32_t ipaddr, int port)
{
    return udp_server(sockaddr_ipv4(ipaddr, port));
}

result<socket> udp_server(const std::string& host, int port)
{
    auto res = sockaddr_ipv4(host, port);
    if (!res) {
	return std::move(res);
    }
    return udp_server(res.value());
}

result<socket> udp_client(const ::sockaddr_in& inaddr)
{
    auto res = socket::udp();
    if (!res) {
	return res;
    }

    auto sock = std::move(res.value());
    auto defer = c7::defer([&sock](){ sock.close(); });

    if (auto res = sock.connect(inaddr); !res) {
	return std::move(res);
    }

    defer.cancel();
    return c7result_ok(std::move(sock));
}

result<socket> udp_client(uint32_t ipaddr, int port)
{
    return udp_client(sockaddr_ipv4(ipaddr, port));
}

result<socket> udp_client(const std::string& host, int port)
{
    auto res = sockaddr_ipv4(host, port);
    if (!res) {
	return std::move(res);
    }
    return udp_client(res.value());
}

result<socket> unix_server(const std::string& path, int backlog)
{
    auto res = socket::unix();
    if (!res) {
	return res;
    }

    auto sock = std::move(res.value());
    auto defer = c7::defer([&sock](){ sock.close(); });

    (void)::unlink(path.c_str());
    if (auto res = sock.bind(path); !res) {
	return std::move(res);
    }
    if (auto res = sock.listen(backlog); !res) {
	return std::move(res);
    }

    defer.cancel();
    return c7result_ok(std::move(sock));
}

result<socket> unix_client(const std::string& path)
{
    auto res = socket::unix();
    if (!res) {
	return res;
    }

    auto sock = std::move(res.value());
    auto defer = c7::defer([&sock](){ sock.close(); });

    if (auto res = sock.connect(path); !res) {
	return std::move(res);
    }

    defer.cancel();
    return c7result_ok(std::move(sock));
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7
