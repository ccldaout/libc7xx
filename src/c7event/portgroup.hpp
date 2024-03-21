/*
 * c7event/portgroup.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_PORTGROUP_HPP_LOADED__
#define C7_EVENT_PORTGROUP_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/port.hpp>
#include <c7event/shared_port.hpp>
#include <c7iters.hpp>


namespace c7::event {


template <typename Port = socket_port>
class portgroup {
private:
    std::vector<Port*> ports_;
    std::vector<std::pair<Port*, io_result>> errs_;

public:
    portgroup() = default;
    portgroup(const portgroup&) = delete;
    portgroup(portgroup&&) = default;
    portgroup& operator=(const portgroup&) = delete;
    portgroup& operator=(portgroup&&) = default;

    void add(Port& port) {
	ports_.push_back(&port);
	port.add_on_close([this, fd=port.fd_number()]() { remove(fd); });
    }
    void remove(Port& port) {
	remove(port.fd_number());
    }
    void remove(int fd) {
	auto it = std::find_if(ports_.begin(), ports_.end(),
			       [fd](auto p){ return (p->fd_number() == fd); });
	if (it != ports_.end()) {
	    ports_.erase(it);
	}
    }

    auto empty() const {
	return ports_.empty();
    }
    auto size() const {
	return ports_.size();
    }
    auto begin() { return ports_.begin(); }
    auto end() { return ports_.end(); }
    auto begin() const { return ports_.begin(); }
    auto end() const { return ports_.end(); }

    void clear_errors() {
	errs_.clear();
    }
    void add_error(Port& port, io_result&& io_res) {
	errs_.emplace_back(&port, std::move(io_res));
    }
    auto& errors() {
	return errs_;
    }
    auto& errors() const {
	return errs_;
    }
    explicit operator bool() const {
	return errs_.empty();
    }
};


template <>
class portgroup<shared_port> {
private:
    using delegate_id = shared_port::delegate_id;

    mutable c7::thread::mutex mutex_;
    std::vector<std::pair<shared_port, delegate_id>> ports_;
    std::vector<std::pair<shared_port, io_result>> errs_;

    static auto extract(std::pair<shared_port, delegate_id>& v) {
	return &v.first;
    }
    static auto cextract(const std::pair<shared_port, delegate_id>& v) {
	return &v.first;
    }

public:
    ~portgroup();
    portgroup() = default;
    portgroup(const portgroup&) = delete;
    portgroup(portgroup&&) = delete;
    portgroup& operator=(const portgroup&) = delete;
    portgroup& operator=(portgroup&&) = delete;

    auto lock() const {
	return mutex_.lock();
    }

    void add(shared_port& port);
    void remove(shared_port& port);
    void remove(int fd);

    auto empty() const {
	return ports_.empty();
    }
    auto size() const {
	return ports_.size();
    }

    auto begin() {
	return make_convert_iter(ports_.begin(), portgroup::cextract);
    }
    auto end() {
	return make_convert_iter(ports_.end(), portgroup::cextract);
    }
    auto begin() const {
	return make_convert_iter(ports_.cbegin(), portgroup::cextract);
    }
    auto end() const {
	return make_convert_iter(ports_.cend(), portgroup::cextract);
    }

    void clear_errors() {
	errs_.clear();
    }
    void add_error(shared_port port, io_result&& io_res) {
	errs_.emplace_back(std::move(port), std::move(io_res));
    }
    std::vector<std::pair<shared_port*, io_result>> errors();
    std::vector<std::pair<const shared_port*, io_result>> errors() const;
    explicit operator bool() const {
	return errs_.empty();
    }
};


template <>
struct lock_traits<portgroup<shared_port>> {
    static inline constexpr bool has_lock = true;
    static auto lock(portgroup<shared_port>& pg) {
	return pg.lock();
    }
    static auto lock_ifimpl(portgroup<shared_port>& pg) {
	return pg.lock();
    }
};


} // namespace c7::event


#endif // c7event/portgroup.hpp
