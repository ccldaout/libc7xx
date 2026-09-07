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
#ifndef C7_EVENT_SHARED_PORT_HPP_LOADED_
#define C7_EVENT_SHARED_PORT_HPP_LOADED_
#include <c7common.hpp>


#define C7_EVENT_SHARED_PORT_IO_OPS	(1U)


#include <c7event/port.hpp>
#include <c7event/traits.hpp>
#include <c7thread/mutex.hpp>


namespace c7::event {


// C7_EVENT_SHARED_PORT_IO_OPS
class port_io_ops {
private:
    friend class shared_port;
    friend class weak_port;

    c7::socket sock_;
    c7::thread::mutex mutex_;
    bool reverse_endian_ = false;

    port_io_ops& operator=(port_io_ops&& o) {
	sock_ = std::move(o.sock_);
	reverse_endian_ = o.reverse_endian_;
	return *this;
    }

protected:
    port_io_ops() = default;

    explicit port_io_ops(c7::socket&& sock):
	sock_(std::move(sock)) {
    }

public:
    virtual ~port_io_ops() = default;

    // non-virtuals

    c7::defer lock() {
	return mutex_.lock();
    }

    void set_different_endian() {
	reverse_endian_ = true;
    }

    bool is_different_endian() const {
	return reverse_endian_;
    }

    c7::socket& socket() {
	return sock_;
    }

    const c7::socket& socket() const {
	return sock_;
    }

    // virtuals

    virtual c7::result<> activate() = 0;
    virtual void close() = 0;
    virtual io_result read_header(void *bufaddr, size_t req_n) = 0;
    virtual io_result read_part(void *bufaddr, size_t req_n) = 0;
    virtual io_result write_entire(::iovec*& iov_io, int& ioc_io) = 0;
    virtual void print(std::ostream& out, const std::string&) const = 0;
};


class shared_port: public port_rw_extention<shared_port> {
private:
    class port_io_ops_default: public port_io_ops {
    private:
	friend class shared_port;
	friend class weak_port;

	explicit port_io_ops_default(c7::socket&& sock):
	    port_io_ops(std::move(sock)) {
	}

	static std::unique_ptr<port_io_ops_default> make() {
	    return std::unique_ptr<port_io_ops_default>(new port_io_ops_default());
	}

	static std::unique_ptr<port_io_ops_default> make(c7::socket&& sock) {
	    return std::unique_ptr<port_io_ops_default>(new port_io_ops_default(std::move(sock)));
	}

    public:
	port_io_ops_default() = default;

	// virtuals

	c7::result<> activate() override {
	    return c7result_ok();
	}

	void close() override {
	    socket().close();
	}

	io_result read_header(void *bufaddr, size_t req_n) override {
	    return socket().read_n(bufaddr, req_n);
	}

	io_result read_part(void *bufaddr, size_t req_n) override {
	    return socket().read_n(bufaddr, req_n);
	}

	io_result write_entire(::iovec*& iov_io, int& ioc_io) override {
	    return socket().write_v(iov_io, ioc_io);
	}

	void print(std::ostream& out, const std::string&) const override {
	    c7::format(out, "shared_port<%{}>", socket());
	}
    };

public:
    using delegate_id = delegate_base::id;

    using base_type = port_rw_extention<shared_port>;
    using base_type::read;
    using base_type::read_n;
    using base_type::write;
    using base_type::write_n;

    shared_port():
	ops_(port_io_ops_default::make()) {
    };

    explicit shared_port(c7::socket&& sock):
	ops_(port_io_ops_default::make(std::move(sock))) {
    }

    explicit shared_port(int fd):
	shared_port(c7::socket{fd}) {
    }

    shared_port(const shared_port&) = default;
    shared_port(shared_port&&) = default;
    shared_port& operator=(const shared_port&) = default;
    shared_port& operator=(shared_port&&) = default;

    bool operator==(const shared_port& o) const {
	return ops_ == o.ops_;
    }
    bool operator!=(const shared_port& o) const {
	return !(*this == o);
    }

    operator bool() const {
	return (ops_ != nullptr);
    }

    // for I/O lock
    auto lock() const {
	return ops_->lock();
    }

    // receiver, acceptor, connector
    int fd_number() const {
	return int(ops_->socket());
    }

    // receiver
    bool is_alive() const {
	return bool(ops_->socket());
    }

    // receiver, acceptor, portgroup
    delegate_id add_on_close(std::function<void()> func) {
	return ops_->socket().on_close.push_back([func](auto&){ func(); });
    }

    // portgroup
    void remove_on_close(delegate_id id) {
	ops_->socket().on_close.remove(id);
    }

    // acceptor
    result<shared_port> accept() {
	if (auto res = ops_->socket().accept(); res) {
	    return c7result_ok(shared_port{std::move(res.value())});
	} else {
	    return res.as_error();
	}
    }

    // connector
    static shared_port tcp();
    static shared_port unix();
    result<> set_nonblocking(bool enable) {
	return ops_->socket().set_nonblocking(enable);
    }
    result<> get_so_error(int *so_error) {
	::socklen_t so_size = sizeof(*so_error);
	return ops_->socket().getsockopt(SOL_SOCKET, SO_ERROR, so_error, &so_size);
    }
    result<> connect(const sockaddr_gen& addr) {
	return ops_->socket().connect(addr);
    }
    result<socket> remake() {
	return ops_->socket().remake();
    }

    // receiver
    void close() {
	return ops_->close();
    }

    // [maybe] user defined service
    void set_different_endian() {
	return ops_->set_different_endian();
    }

    // multipart_msgbuf
    bool is_different_endian() {
	return ops_->is_different_endian();
    }

    // multipart_msgbuf: read entire message by combining read_header()
    //                   with multiple read_part().
    io_result read_header(void *bufaddr, size_t req_n) {	// C7_EVENT_PORT_API_MSGBUF
	return ops_->read_header(bufaddr, req_n);
    }
    io_result read_part(void *bufaddr, size_t req_n) {		// C7_EVENT_PORT_API_MSGBUF
	return ops_->read_part(bufaddr, req_n);
    }

    // multipart_msgbuf: write entire message (header, data, ...)
    io_result write_entire(::iovec*& iov_io, int& ioc_io) {	// C7_EVENT_PORT_API_MSGBUF
	return ops_->write_entire(iov_io, ioc_io);
    }

    // formattable
    void print(std::ostream& out, const std::string&) const;

    // for the user's code
    result<> replace_ops(std::unique_ptr<port_io_ops> ops) {	// C7_EVENT_SHARED_PORT_IO_OPS
	*ops = std::move(*ops_);
	ops_ = std::move(ops);
	return ops_->activate();
    }
    result<> set_cloexec(bool enable) {
	return ops_->socket().set_cloexec(enable);
    }
    result<> tcp_keepalive(bool enable) {
	return ops_->socket().tcp_keepalive(enable);
    }
    result<> tcp_nodelay(bool enable) {
	return ops_->socket().tcp_nodelay(enable);
    }
    result<> set_rcvbuf(int nbytes) {
	// server:before listen, client:before conenct
	return ops_->socket().set_rcvbuf(nbytes);
    }
    result<> set_sndbuf(int nbytes) {
	return ops_->socket().set_sndbuf(nbytes);
    }
    result<> set_sndtmo(c7::usec_t timeout) {
	return ops_->socket().set_sndtmo(timeout);
    }
    result<> set_rcvtmo(c7::usec_t timeout) {
	return ops_->socket().set_rcvtmo(timeout);
    }
    result<> shutdown_r() {
	return ops_->socket().shutdown_r();
    }
    result<> shutdown_w() {
	return ops_->socket().shutdown_w();
    }
    result<> shutdown_rw() {
	return ops_->socket().shutdown_rw();
    }
    socket *operator->() {
	return &ops_->socket();
    }
    const socket *operator->() const {
	return &ops_->socket();
    }
    socket release() {						// C7_EVENT_PORT_API_RELEASE
	return std::move(ops_->socket());
    }

    // raw socket I/O
    result<size_t> read(void *bufaddr, size_t size) {
	return ops_->socket().read(bufaddr, size);
    }
    result<size_t> write(const void *bufaddr, size_t size) {
	return ops_->socket().write(bufaddr, size);
    }
    io_result read_n(void *bufaddr, size_t req_n) {
	return ops_->socket().read_n(bufaddr, req_n);
    }
    io_result write_n(const void *bufaddr, size_t req_n) {
	return ops_->socket().write_n(bufaddr, req_n);
    }
    io_result write_v(::iovec*& iov_io, int& ioc_io) {
	return ops_->socket().write_v(iov_io, ioc_io);
    }

private:
    friend class weak_port;

    std::shared_ptr<port_io_ops> ops_;

    explicit shared_port(std::shared_ptr<port_io_ops>&& pimpl):
	ops_(std::move(pimpl)) {
    }
};


class weak_port {
private:
    std::weak_ptr<port_io_ops> w_ops_;

public:
    weak_port() = default;
    weak_port(const weak_port& wp): w_ops_(wp.w_ops_) {}
    weak_port(weak_port&& wp): w_ops_(std::move(wp.w_ops_)) {}
    weak_port& operator=(const weak_port& wp) {
	w_ops_ = wp.w_ops_;
	return *this;
    }
    weak_port& operator=(weak_port&& wp) {
	if (this != &wp) {
	    w_ops_ = std::move(wp.w_ops_);
	}
	return *this;
    }
    weak_port(const shared_port& sp): w_ops_(sp.ops_) {}
    weak_port& operator=(const shared_port& sp) {
	w_ops_ = sp.ops_;
	return *this;
    }

    bool operator==(const weak_port& o) const {
	return w_ops_.lock() == o.w_ops_.lock();
    }
    bool operator!=(const weak_port& o) const {
	return !(*this == o);
    }
    bool operator==(const shared_port& o) const {
	return w_ops_.lock() == o.ops_;
    }
    bool operator!=(const shared_port& o) const {
	return !(*this == o);
    }

    void reset() {
	w_ops_.reset();
    }

    void print(std::ostream& out, const std::string&) const;

    shared_port lock() {
	return shared_port(std::move(w_ops_.lock()));
    }
};


inline bool operator==(const shared_port sp, const weak_port& wp)
{
    return wp == sp;
}

inline bool operator!=(const shared_port sp, const weak_port& wp)
{
    return wp != sp;
}


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
