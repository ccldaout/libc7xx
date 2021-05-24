/*
 * c7event/port.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_PORT_HPP_LOADED__
#define C7_EVENT_PORT_HPP_LOADED__
#include <c7common.hpp>


#include <c7socket.hpp>
#include <variant>


namespace c7::event {


typedef std::variant<std::string, ::sockaddr_in> socket_addr;


class port_rw_extention {
    template <typename T> result<size_t> read(T *buf) {
	return read(buf, sizeof(T));
    }
    template <typename T, size_t N> result<size_t> read(T (*buf)[N]) {
	return read(buf, sizeof(T)*N);
    }
    template <typename T> result<size_t> write(const T *buf) {
	return write(buf, sizeof(T));
    }
    template <typename T, size_t N> result<size_t> write(const T (*buf)[N]) {
	return write(buf, sizeof(T)*N);
    }

    template <typename T> io_result read_n(T *buf) {
	return read_n(buf, sizeof(T));
    }
    template <typename T, size_t N> io_result read_n(T (*buf)[N]) {
	return read_n(buf, sizeof(T)*N);
    }
    template <typename T> io_result write_n(const T *buf) {
	return write_n(buf, sizeof(T));
    }
    template <typename T, size_t N> io_result write_n(const T (*buf)[N]) {
	return write_n(buf, sizeof(T)*N);
    }
};


class socket_port: public port_rw_extention {
public:
    socket_port() = default;
    explicit socket_port(c7::socket&& sock);
    explicit socket_port(int fd);
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

    // for the user's code
    result<size_t> read(void *bufaddr, size_t size);
    result<size_t> write(const void *bufaddr, size_t size);
    io_result write_n(const void *bufaddr, size_t req_n);
    result<> set_cloexec(bool enable);
    result<> tcp_keepalive(bool enable);
    result<> tcp_nodelay(bool enable);
    result<> set_rcvbuf(int nbytes);	// server:before listen, client:before conenct
    result<> set_sndbuf(int nbytes);
    result<> set_sndtmo(c7::usec_t timeout);
    result<> set_rcvtmo(c7::usec_t timeout);
    result<> shutdown_r();
    result<> shutdown_w();
    result<> shutdown_rw();

private:
    c7::socket sock_;
    bool reverse_endian_ = false;
};


} // namespace c7::event


#endif // c7event/port.hpp
