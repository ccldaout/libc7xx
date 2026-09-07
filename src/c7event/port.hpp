/*
 * c7event/port.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_PORT_HPP_LOADED_
#define C7_EVENT_PORT_HPP_LOADED_
#include <c7common.hpp>


#include <c7socket.hpp>
#include <variant>


#define C7_EVENT_PORT_API_RELEASE	(1U)
#define C7_EVENT_PORT_API_MSGBUF	(1U)


namespace c7::event {


template <typename D>
struct port_rw_extention {
    template <typename T> result<size_t> read(T *buf) {
	return static_cast<D*>(this)->read(buf, sizeof(T));
    }
    template <typename T, size_t N> result<size_t> read(T (*buf)[N]) {
	return static_cast<D*>(this)->read(buf, sizeof(T)*N);
    }
    template <typename T> result<size_t> write(const T *buf) {
	return static_cast<D*>(this)->write(buf, sizeof(T));
    }
    template <typename T, size_t N> result<size_t> write(const T (*buf)[N]) {
	return static_cast<D*>(this)->write(buf, sizeof(T)*N);
    }

    template <typename T> io_result read_n(T *buf) {
	return static_cast<D*>(this)->read_n(buf, sizeof(T));
    }
    template <typename T, size_t N> io_result read_n(T (*buf)[N]) {
	return static_cast<D*>(this)->read_n(buf, sizeof(T)*N);
    }
    template <typename T> io_result write_n(const T *buf) {
	return static_cast<D*>(this)->write_n(buf, sizeof(T));
    }
    template <typename T, size_t N> io_result write_n(const T (*buf)[N]) {
	return static_cast<D*>(this)->write_n(buf, sizeof(T)*N);
    }
};


class socket_port: public port_rw_extention<socket_port> {
public:
    using delegate_id = delegate_base::id;

    using port_rw_extention<socket_port>::read;
    using port_rw_extention<socket_port>::read_n;
    using port_rw_extention<socket_port>::write;
    using port_rw_extention<socket_port>::write_n;

    socket_port() = default;
    explicit socket_port(c7::socket&& sock): sock_(std::move(sock)) {}
    explicit socket_port(int fd): sock_(fd) {}
    socket_port(socket_port&& o):
	sock_(std::move(o.sock_)), reverse_endian_(o.reverse_endian_) {
	o.reverse_endian_ = false;
    }
    socket_port& operator=(socket_port&& o) {
	if (this != &o) {
	    sock_ = std::move(o.sock_);
	    reverse_endian_ = o.reverse_endian_;
	    o.reverse_endian_ = false;
	}
	return *this;
    }

    socket_port(const socket_port&) = delete;
    socket_port& operator=(const socket_port&) = delete;

    // receiver, acceptor, connector
    int fd_number() const {
	return int(sock_);
    }

    // receiver
    bool is_alive() const {
	return bool(sock_);
    }

    // receiver, acceptor, portgroup
    delegate_id add_on_close(std::function<void()> func) {
	return sock_.on_close.push_back([func](auto&){ func(); });
    }

    // portgroup
    void remove_on_close(delegate_id id) {
	sock_.on_close.remove(id);
    }

    // acceptor
    result<socket_port> accept() {
	if (auto res = sock_.accept(); !res) {
	    return c7result_err(std::move(res));
	} else {
	    return c7result_ok(socket_port(std::move(res.value())));
	}
    }

    // connector
    static socket_port tcp();
    static socket_port unix();
    result<> set_nonblocking(bool enable) {
	return sock_.set_nonblocking(enable);
    }
    result<> get_so_error(int *so_error) {
	::socklen_t so_size = sizeof(*so_error);
	return sock_.getsockopt(SOL_SOCKET, SO_ERROR, so_error, &so_size);
    }
    result<> connect(const sockaddr_gen& addr) {
	return sock_.connect(addr);
    }
    result<socket> remake() {
	return sock_.remake();
    }

    // receiver
    void close() {
	sock_.close();
    }

    // [maybe] user defined service
    void set_different_endian() {
	reverse_endian_ = true;
    }

    // multipart_msgbuf
    bool is_different_endian() {
	return reverse_endian_;
    }

    // multipart_msgbuf: read entire message by combining read_header()
    //                   with multiple read_part().
    io_result read_header(void *bufaddr, size_t req_n) {	// C7_EVENT_PORT_API_MSGBUF
	return sock_.read_n(bufaddr, req_n);
    }
    io_result read_part(void *bufaddr, size_t req_n) {		// C7_EVENT_PORT_API_MSGBUF
	return sock_.read_n(bufaddr, req_n);
    }

    // multipart_msgbuf: write entire message (header, data, ...)
    io_result write_entire(::iovec*& iov_io, int& ioc_io) {	// C7_EVENT_PORT_API_MSGBUF
	return sock_.write_v(iov_io, ioc_io);
    }

    // formattable
    void print(std::ostream& out, const std::string&) const {
	c7::format(out, "socket_port<%{}>", sock_);
    }

    // for the user's code
    result<> set_cloexec(bool enable) {
	return sock_.set_cloexec(enable);
    }
    result<> tcp_keepalive(bool enable) {
	return sock_.tcp_keepalive(enable);
    }
    result<> tcp_nodelay(bool enable) {
	return sock_.tcp_nodelay(enable);
    }
    result<> set_rcvbuf(int nbytes) {	// server:before listen, client:before conenct
	return sock_.set_rcvbuf(nbytes);
    }
    result<> set_sndbuf(int nbytes) {
	return sock_.set_sndbuf(nbytes);
    }
    result<> set_sndtmo(c7::usec_t timeout) {
	return sock_.set_sndtmo(timeout);
    }
    result<> set_rcvtmo(c7::usec_t timeout) {
	return sock_.set_rcvtmo(timeout);
    }
    result<> shutdown_r() {
	return sock_.shutdown_r();
    }
    result<> shutdown_w() {
	return sock_.shutdown_w();
    }
    result<> shutdown_rw() {
	return sock_.shutdown_rw();
    }
    c7::socket *operator->() {
	return &sock_;
    }
    const c7::socket *operator->() const {
	return &sock_;
    }
    c7::socket release() {		// C7_EVENT_PORT_API_RELEASE
	return std::move(sock_);
    }

    // raw socket I/O
    result<size_t> read(void *bufaddr, size_t size) {
	return sock_.read(bufaddr, size);
    }
    io_result read_n(void *bufaddr, size_t req_n) {
	return sock_.read_n(bufaddr, req_n);
    }
    result<size_t> write(const void *bufaddr, size_t size) {
	return sock_.write(bufaddr, size);
    }
    io_result write_n(const void *bufaddr, size_t req_n) {
	return sock_.write_n(bufaddr, req_n);
    }
    io_result write_v(::iovec*& iov_io, int& ioc_io) {
	return sock_.write_v(iov_io, ioc_io);
    }

private:
    c7::socket sock_;
    bool reverse_endian_ = false;
};


} // namespace c7::event


#endif // c7event/port.hpp
