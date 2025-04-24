/*
 * c7socket.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/socket.h>
#include <cstring>
#include <random>
#include <c7defer.hpp>
#include <c7socket.hpp>
#include <c7utils/time.hpp>


static const char *inaddr_name(const ::sockaddr_in& inaddr)
{
    static thread_local char namebuf[INET_ADDRSTRLEN];
    auto addr = reinterpret_cast<const ::sockaddr *>(&inaddr);
    (void)getnameinfo(addr, sizeof(inaddr),
		      namebuf, sizeof(namebuf),
		      nullptr, 0,
		      0);
    return namebuf;
}


namespace c7 {


/*----------------------------------------------------------------------------
                                socket address
----------------------------------------------------------------------------*/

void sockaddr_gen::clear()
{
    std::memset(static_cast<void *>(this), 0, sizeof(*this));
}

void sockaddr_gen::set_port(int port)
{
    ipv4.sin_port = htons(port);
}

bool sockaddr_gen::is_ipv4() const
{
    return (base.sa_family == AF_INET);
}

bool sockaddr_gen::is_unix() const
{
    return (base.sa_family == AF_UNIX);
}

::socklen_t sockaddr_gen::socklen() const
{
    if (is_ipv4()) {
	return sizeof(ipv4);
    } else {
	return (unix.sun_path[0] == 0) ? sizeof(unix) : SUN_LEN(&unix);
    }
}

void sockaddr_gen::print(std::ostream& out, const std::string&) const
{
    if (is_ipv4()) {
	c7::format(out, "ipv4<%{}:%{}>", inaddr_name(ipv4), ntohs(ipv4.sin_port));
    } else {
	const auto& p = unix.sun_path;
	c7::format(out, "unix<%{}%{}>", *p == 0 ? '!' : *p, p+1);
    }
}

result<sockaddr_gen> sockaddr_unix(const std::string& path)
{
    sockaddr_gen addr;
    auto& unaddr = addr.unix;
    (void)std::memset(&unaddr, 0, sizeof(unaddr));
    if (sizeof(unaddr.sun_path) < path.size() + 1) {
	return c7result_err(EINVAL, "sockaddr_un: too long path: %{}", path);
    }
    (void)std::memcpy(unaddr.sun_path, path.c_str(), path.size());
    unaddr.sun_family = AF_UNIX;
    return c7result_ok(addr);
}

sockaddr_gen sockaddr_ipv4(uint32_t ipaddr, int port)
{
    sockaddr_gen addr;
    auto& inaddr = addr.ipv4;
    (void)std::memset(&inaddr, 0, sizeof(inaddr));
    inaddr.sin_addr.s_addr = ipaddr;
    inaddr.sin_family = AF_INET;
    inaddr.sin_port   = htons(port);
    return addr;
}

result<sockaddr_gen> sockaddr_ipv4(const std::string& host, int port)
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
	ipaddr = reinterpret_cast<::sockaddr_in *>(result->ai_addr)->sin_addr.s_addr;
	::freeaddrinfo(result);
    }
    return c7result_ok(sockaddr_ipv4(ipaddr, port));
}


/*----------------------------------------------------------------------------
                                    socket
----------------------------------------------------------------------------*/

void socket::setup_str() const
{
    if (auto res = self(); !res) {
	name_ = "?";
    } else if (res.value().is_ipv4()) {
	auto& self = res.value().ipv4;
	if (auto res = peer(); res) {
	    auto& peer = res.value().ipv4;
	    name_ = c7::format("peer:%{}:%{} self:%{}",
			       inaddr_name(peer), ntohs(peer.sin_port), ntohs(self.sin_port));
	} else {
	    name_ = c7::format("peer:? self:%{}:%{}",
			       inaddr_name(self), ntohs(self.sin_port));
	}
    } else {
	name_ = c7::format("%{}", res.value());
    }
}

result<socket> socket::make_socket(int domain, int type, int protocol)
{
    int fd = ::socket(domain, type, protocol);
    if (fd == C7_SYSERR) {
	return c7result_err(errno, "socket(%{}, %{}, %{}) failed", domain, type, protocol);
    }
    auto sock = c7::socket(fd);
    return c7result_ok(std::move(sock));
}

result<socket> socket::tcp()
{
    return socket::make_socket(AF_INET, SOCK_STREAM);
}

result<socket> socket::udp()
{
    return socket::make_socket(AF_INET, SOCK_DGRAM);
}

result<socket> socket::unix()
{
    return socket::make_socket(AF_UNIX, SOCK_STREAM);
}

result<socket> socket::unix_dg()
{
    return socket::make_socket(AF_UNIX, SOCK_DGRAM);
}

result<> socket::bind(const sockaddr_gen& addr)
{
    int reuse = 1;
    (void)::setsockopt(fdnum_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (::bind(fdnum_, &addr.base, addr.socklen()) == C7_SYSERR) {
	return c7result_err(errno, "bind(%{}, %{}) failed", *this, addr);
    }
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
	return c7result_err(std::move(res), "bind(%{}, %{}, %{}) failed", fdnum_, host, port);
    }
    return socket::bind(res.value());

}

result<> socket::bind(const std::string& path)	// UNIX domain
{
    auto res = sockaddr_unix(path);
    if (!res) {
	return c7result_err(std::move(res), "bind(%{}, %{}) failed", fdnum_, path);
    }
    auto& addr = res.value();
    if (::bind(fdnum_, &addr.base, addr.socklen()) == C7_SYSERR) {
	return c7result_err(errno, "bind(%{}, %{}) failed", fdnum_, path);
    }
    return c7result_ok();
}

result<> socket::connect(const sockaddr_gen& addr)
{
    (void)tcp_keepalive(true);			// called here for non-blocking connect.
    if (::connect(fdnum_, &addr.base, addr.socklen()) == C7_SYSERR) {
	return c7result_err(errno, "connect(%{}, %{}) failed", fdnum_, addr);
    }
    if (addr.is_unix()) {
	// getsockname return sun_path as empty string in case of UNIX domain socket
	// active opened. therefore name_ is assigned here.
	const auto& p = addr.unix.sun_path;
	name_ = c7::format("unix<%{}%{}>", *p == 0 ? '!' : *p, p+1);
    }
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
	return c7result_err(std::move(res), "connect(%{}, %{}, %{}) failed", fdnum_, host, port);
    }
    return socket::connect(res.value());
}

result<> socket::connect(const std::string& path)	// UNIX domain
{
    auto res = sockaddr_unix(path);
    if (!res) {
	return c7result_err(std::move(res), "connect(%{}, %{}) failed", fdnum_, path);
    }
    return connect(res.value());
}

result<> socket::listen(int backlog)
{
    if (::listen(fdnum_, backlog) == C7_SYSERR) {
	return c7result_err(errno, "listen(%{}, %{}) failed", fdnum_, backlog);
    }
    return c7result_ok();
}

result<socket> socket::accept()
{
    int newfd = ::accept(fdnum_, nullptr, nullptr);
    if (newfd == C7_SYSERR) {
	return c7result_err(errno, "accept(%{}) failed", fdnum_);
    }
    auto newsock = c7::socket(newfd);
    (void)newsock.tcp_keepalive(true);
    return c7result_ok(std::move(newsock));
}

result<sockaddr_gen> socket::self() const
{
    sockaddr_gen addr;
    socklen_t n = sizeof(addr);
    addr.clear();
    int ret = ::getsockname(fdnum_, &addr.base, &n);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "getsockname(%{}) failed", fdnum_);
    }
    return c7result_ok(addr);
}

result<sockaddr_gen> socket::peer() const
{
    sockaddr_gen addr;
    socklen_t n = sizeof(addr);
    addr.clear();
    int ret = ::getpeername(fdnum_, &addr.base, &n);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "getpeername(%{}) failed", fdnum_);
    }
    return c7result_ok(addr);
}

result<> socket::getsockopt(int level, int optname, void *optval, socklen_t *optlen) const
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

io_result socket::recvfrom(void *buf, size_t bufsize, sockaddr_gen& addr, int flags)
{
    socklen_t len = sizeof(addr);
    ssize_t n = ::recvfrom(fdnum_, buf, bufsize, flags, &addr.base, &len);
    if (n > 0) {
	return io_result::ok(n);
    } else if (n == 0) {
	return io_result::closed(*this);
    } else {
	return io_result::error(*this);
    }
}

io_result socket::sendto(const void *buf, size_t bufsize, const sockaddr_gen& addr, int flags)
{
    ssize_t n = ::sendto(fdnum_, buf, bufsize, flags, &addr.base, addr.socklen());
    if (static_cast<size_t>(n) == bufsize) {
	return io_result::ok(n);
    } else if (n > 0) {
	return io_result::incomp(*this, n, bufsize - n);
    } else if (n == 0) {
	return io_result::closed(*this);
    } else {
	return io_result::error(*this);
    }
}

std::string socket::to_string(const std::string&) const
{
    if (name_.empty()) {
	setup_str();
    }
    const char *s = name_[0] == '?' ? "fd" : "socket";
    return c7::format("%{}<%{} %{}>", s, fdnum_, name_);
}

void socket::print(std::ostream& out, const std::string&) const
{
    if (name_.empty()) {
	setup_str();
    }
    const char *s = name_[0] == '?' ? "fd" : "socket";
    c7::format(out, "%{}<%{} %{}>", s, fdnum_, name_);
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

result<socket> tcp_server(const sockaddr_gen& addr, int rcvbuf_size, int backlog)
{
    auto res = socket::tcp();
    if (!res) {
	return res;
    }

    auto sock = std::move(res.value());
    auto defer = c7::defer([&sock](){ sock.close(); });

    if (auto res = sock.bind(addr); !res) {
	return res.as_error();
    }
    if (rcvbuf_size > 0) {
	if (auto res = sock.set_rcvbuf(rcvbuf_size); !res) {
	    return res.as_error();
	}
    }
    if (auto res = sock.listen(backlog); !res) {
	return res.as_error();
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
	return res.as_error();
    }
    return tcp_server(res.value(), rcvbuf_size, backlog);
}

result<socket> tcp_client(const sockaddr_gen& addr, int rcvbuf_size)
{
    auto res = socket::tcp();
    if (!res) {
	return res;
    }

    auto sock = std::move(res.value());
    auto defer = c7::defer([&sock](){ sock.close(); });

    if (rcvbuf_size > 0) {
	if (auto res = sock.set_rcvbuf(rcvbuf_size); !res) {
	    return res.as_error();
	}
    }
    if (auto res = sock.connect(addr); !res) {
	return res.as_error();
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
	return res.as_error();
    }
    return tcp_client(res.value(), rcvbuf_size);
}

result<socket> udp_binded(const sockaddr_gen& addr)
{
    auto res = socket::udp();
    if (!res) {
	return res.as_error();
    }

    auto sock = std::move(res.value());
    auto defer = c7::defer([&sock](){ sock.close(); });

    if (auto res = sock.bind(addr); !res) {
	return res.as_error();
    }

    defer.cancel();
    return c7result_ok(std::move(sock));
}

result<socket> udp_binded(uint32_t ipaddr, int port)
{
    return udp_binded(sockaddr_ipv4(ipaddr, port));
}

result<socket> udp_binded(const std::string& host, int port)
{
    auto res = sockaddr_ipv4(host, port);
    if (!res) {
	return res.as_error();
    }
    return udp_binded(res.value());
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
	return res.as_error();
    }
    if (auto res = sock.listen(backlog); !res) {
	return res.as_error();
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
	return res.as_error();
    }

    defer.cancel();
    return c7result_ok(std::move(sock));
}

result<socket> unix_dg_binded(const std::string& path)
{
    auto res = socket::unix_dg();
    if (!res) {
	return res;
    }

    auto sock = std::move(res.value());
    auto defer = c7::defer([&sock](){ sock.close(); });

    (void)::unlink(path.c_str());
    if (auto res = sock.bind(path); !res) {
	return res.as_error();
    }

    defer.cancel();
    return c7result_ok(std::move(sock));
}

result<socket> unix_dg_binded()
{
    static const uint64_t seed{std::random_device{}()};
    static std::atomic<uint64_t> counter;
    auto path = c7::format(" .%{}.%{}@ccldaout", seed, counter++);
    path[0] = 0;
    return unix_dg_binded(path);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7
