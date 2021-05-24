/*
 * c7event/portgroup.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_PORTGROUP_HPP_LOADED__
#define C7_EVENT_PORTGROUP_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/port.hpp>


namespace c7::event {


template <typename Port = socket_port>
class portgroup {
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
    auto& errors() const {
	return errs_;
    }
    explicit operator bool() const {
	return errs_.empty();
    }

private:
    std::vector<Port*> ports_;
    std::vector<std::pair<Port*, io_result>> errs_;
};


} // namespace c7::event


#endif // c7event/portgroup.hpp
