/*
 * c7event/shared_port.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_SHARED_PORT_HPP_LOADED__
#define C7_EVENT_SHARED_PORT_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/port.hpp>
#include <c7event/traits.hpp>
#include <c7thread/mutex.hpp>


namespace c7::event {


class shared_port: public port_rw_extention<shared_port> {
private:
    friend class weak_port;
    struct impl {
	~impl() {}
	impl() = default;
	explicit impl(c7::socket&& sock);
	explicit impl(int fd);
	explicit impl(socket_port&& port);
	socket_port port;
	c7::thread::mutex io_mutex;	// for I/O mutex
    };
    std::shared_ptr<impl> pimpl_;

public:
    using delegate_id = socket_port::delegate_id;

    using base_type = port_rw_extention<shared_port>;
    using base_type::read;
    using base_type::read_n;
    using base_type::write;
    using base_type::write_n;

    shared_port(): pimpl_(new impl) {};
    explicit shared_port(std::shared_ptr<impl>&& pimpl);
    explicit shared_port(c7::socket&& sock);
    explicit shared_port(int fd);
    explicit shared_port(socket_port&& port);
    shared_port(const shared_port&) = default;
    shared_port(shared_port&& o) = default;
    shared_port& operator=(const shared_port&) = default;
    shared_port& operator=(shared_port&&) = default;

    operator bool() const {
	return (pimpl_ != nullptr);
    }

    // for I/O lock
    auto lock() const {
	return pimpl_->io_mutex.lock();
    }

    // connector
    static shared_port tcp();
    static shared_port unix();

    // receiver, acceptor, connector
    int fd_number() const {
	return pimpl_->port.fd_number();
    }

    // receiver
    bool is_alive() const {
	return pimpl_->port.is_alive();
    }

    // receiver, acceptor
    template <typename Func> delegate_id
    add_on_close(Func&& func) {
	return pimpl_->port.add_on_close(std::forward<Func>(func));
    }

    void remove_on_close(delegate_id id) {
	pimpl_->port.remove_on_close(id);
    }

    // connector
    result<> set_nonblocking(bool enable) {
	return pimpl_->port.set_nonblocking(enable);
    }

    // acceptor
    result<shared_port> accept();

    // connector
    result<> get_so_error(int *so_error) {
	return pimpl_->port.get_so_error(so_error);
    }

    // connector
    result<> connect(const sockaddr_gen& addr) {
	return pimpl_->port.connect(addr);
    }

    // receiver
    void close() {
	return pimpl_->port.close();
    }

    // [maybe] user defined service
    void set_different_endian() {
	return pimpl_->port.set_different_endian();
    }

    // multipart_msgbuf
    bool is_different_endian() {
	return pimpl_->port.is_different_endian();
    }

    // multipart_msgbuf
    io_result read_n(void *bufaddr, size_t req_n) {
	return pimpl_->port.read_n(bufaddr, req_n);
    }

    // multipart_msgbuf
    io_result write_v(::iovec*& iov_io, int& ioc_io) {
	return pimpl_->port.write_v(iov_io, ioc_io);
    }

    // formattable
    void print(std::ostream& out, const std::string&) const;

    // for the user's code
    result<size_t> read(void *bufaddr, size_t size) {
	return pimpl_->port.read(bufaddr, size);
    }
    result<size_t> write(const void *bufaddr, size_t size) {
	return pimpl_->port.write(bufaddr, size);
    }
    io_result write_n(const void *bufaddr, size_t req_n) {
	return pimpl_->port.write_n(bufaddr, req_n);
    }
    result<> set_cloexec(bool enable) {
	return pimpl_->port.set_cloexec(enable);
    }
    result<> tcp_keepalive(bool enable) {
	return pimpl_->port.tcp_keepalive(enable);
    }
    result<> tcp_nodelay(bool enable) {
	return pimpl_->port.tcp_nodelay(enable);
    }
    result<> set_rcvbuf(int nbytes) {	// server:before listen, client:before conenct
	return pimpl_->port.set_rcvbuf(nbytes);
    }
    result<> set_sndbuf(int nbytes) {
	return pimpl_->port.set_sndbuf(nbytes);
    }
    result<> set_sndtmo(c7::usec_t timeout) {
	return pimpl_->port.set_sndtmo(timeout);
    }
    result<> set_rcvtmo(c7::usec_t timeout) {
	return pimpl_->port.set_rcvtmo(timeout);
    }
    result<> shutdown_r() {
	return pimpl_->port.shutdown_r();
    }
    result<> shutdown_w() {
	return pimpl_->port.shutdown_w();
    }
    result<> shutdown_rw() {
	return pimpl_->port.shutdown_rw();
    }
    c7::socket *operator->() {
	return pimpl_->port.operator->();
    }
    const c7::socket *operator->() const {
	return pimpl_->port.operator->();
    }
};


class weak_port {
private:
    std::weak_ptr<shared_port::impl> w_pimpl_;

public:
    weak_port() = default;
    weak_port(const weak_port& wp): w_pimpl_(wp.w_pimpl_) {}
    weak_port(weak_port&& wp): w_pimpl_(std::move(wp.w_pimpl_)) {}
    weak_port& operator=(const weak_port& wp) {
	w_pimpl_ = wp.w_pimpl_;
	return *this;
    }
    weak_port& operator=(weak_port&& wp) {
	if (this != &wp) {
	    w_pimpl_ = std::move(wp.w_pimpl_);
	}
	return *this;
    }
    weak_port(const shared_port& sp): w_pimpl_(sp.pimpl_) {}
    weak_port& operator=(const shared_port& sp) {
	w_pimpl_ = sp.pimpl_;
	return *this;
    }

    void reset() {
	w_pimpl_.reset();
    }

    shared_port lock() {
	return shared_port(std::move(w_pimpl_.lock()));
    }
};


template <>
struct lock_traits<shared_port> {
    static inline constexpr bool has_lock = true;
    static auto lock(shared_port& port) {
	return port.lock();
    }
    static auto lock_ifimpl(shared_port& port) {
	return port.lock();
    }
};


} // namespace c7::event


#endif // c7event/port.hpp
