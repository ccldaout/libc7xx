/*
 * c7event/port.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7event/port.hpp>


namespace c7::event {


socket_port::socket_port(c7::socket&& sock): sock_(std::move(sock)) {}

socket_port::socket_port(int fd): sock_(fd) {}

socket_port::socket_port(socket_port&& o):
    sock_(std::move(o.sock_)), reverse_endian_(o.reverse_endian_)
{
    o.reverse_endian_ = false;
}

socket_port& socket_port::operator=(socket_port&& o)
{
    if (this != &o) {
	sock_ = std::move(o.sock_);
	reverse_endian_ = o.reverse_endian_;
	o.reverse_endian_ = false;
    }
    return *this;
}

socket_port socket_port::tcp()
{
    if (auto res = c7::socket::tcp(); res) {
	return socket_port(std::move(res.value()));
    }
    return socket_port(c7::socket());
}

socket_port socket_port::unix()
{
    if (auto res = c7::socket::unix(); res) {
	return socket_port(std::move(res.value()));
    }
    return socket_port(c7::socket());
}

int socket_port::fd_number() const
{
    return int(sock_);
}

bool socket_port::is_alive() const
{
    return bool(sock_);
}

void socket_port::set_different_endian()
{
    reverse_endian_ = true;
}

bool socket_port::is_different_endian()
{
    return reverse_endian_;
}

result<> socket_port::set_nonblocking(bool enable)
{
    return sock_.set_nonblocking(enable);
}

result<socket_port> socket_port::accept()
{
    if (auto res = sock_.accept(); !res) {
	return c7result_err(std::move(res));
    } else {
	return c7result_ok(socket_port(std::move(res.value())));
    }
}

result<> socket_port::get_so_error(int *so_error)
{
    ::socklen_t so_size = sizeof(*so_error);
    return sock_.getsockopt(SOL_SOCKET, SO_ERROR, so_error, &so_size);
}

result<> socket_port::connect(const sockaddr_gen& addr)
{
    return sock_.connect(addr);
}

io_result socket_port::read_n(void *bufaddr, size_t req_n)
{
    return sock_.read_n(bufaddr, req_n);
}

io_result socket_port::write_v(::iovec*& iov_io, int& ioc_io)
{
    return sock_.write_v(iov_io, ioc_io);
}

void socket_port::close()
{
    sock_.close();
}

void socket_port::print(std::ostream& out, const std::string& spec) const
{
    sock_.print(out, spec);
}

result<size_t> socket_port::read(void *bufaddr, size_t size)
{
    return sock_.read(bufaddr, size);
}

result<size_t> socket_port::write(const void *bufaddr, size_t size)
{
    return sock_.write(bufaddr, size);
}

io_result socket_port::write_n(const void *bufaddr, size_t req_n)
{
    return sock_.write_n(bufaddr, req_n);
}

result<> socket_port::set_cloexec(bool enable)
{
    return sock_.set_cloexec(enable);
}

result<> socket_port::tcp_keepalive(bool enable)
{
    return sock_.tcp_keepalive(enable);
}

result<> socket_port::tcp_nodelay(bool enable)
{
    return sock_.tcp_nodelay(enable);
}

result<> socket_port::set_rcvbuf(int nbytes)
{
    return sock_.set_rcvbuf(nbytes);
}

result<> socket_port::set_sndbuf(int nbytes)
{
    return sock_.set_sndbuf(nbytes);
}

result<> socket_port::set_sndtmo(c7::usec_t timeout)
{
    return sock_.set_sndtmo(timeout);
}

result<> socket_port::set_rcvtmo(c7::usec_t timeout)
{
    return sock_.set_rcvtmo(timeout);
}

result<> socket_port::shutdown_r()
{
    return sock_.shutdown_r();
}

result<> socket_port::shutdown_w()
{
    return sock_.shutdown_w();
}

result<> socket_port::shutdown_rw()
{
    return sock_.shutdown_rw();
}


} // namespace c7::event
