/*
 * c7event/ext/delegate.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_EXT_DELEGATE_HPP_LOADED_
#define C7_EVENT_EXT_DELEGATE_HPP_LOADED_
#include <c7common.hpp>


#include <c7event/service.hpp>
#include <list>


namespace c7::event::ext {


template <typename BaseService>
class delegate: public BaseService {
public:
    using port_type    = typename BaseService::port_type;
    using msgbuf_type  = typename BaseService::msgbuf_type;
    using service_type = service_interface<msgbuf_type, port_type>;

private:
    class broker {
    private:
	friend class delegate;

	c7::thread::mutex mutex_;
	std::list<std::pair<int, std::weak_ptr<service_type>>> services_;

	auto services() {
	    std::vector<std::shared_ptr<service_type>> spv;
	    auto unlock = mutex_.lock();
	    for (auto it = services_.begin(); it != services_.end();) {
		auto sp = (*it).second.lock();
		if (!sp) {
		    it = services_.erase(it);
		} else {
		    ++it;
		    spv.push_back(std::move(sp));
		}
	    }
	    unlock();
	    return spv;
	}

    public:
	void install(int priority, std::shared_ptr<service_type> sp) {
	    auto unlock = mutex_.lock();
	    auto it = std::find_if(services_.begin(), services_.end(),
				   [priority](auto& t){ return t.first < priority; });
	    if (it == services_.end()) {
		services_.emplace_back(priority, sp);
	    } else {
		services_.insert(it, std::make_pair(priority, std::move(sp)));
	    }
	}

	void operator+=(std::shared_ptr<service_type> sp) {
	    install(0, std::move(sp));
	}
    };

public:
    broker ext_delegate;

    delegate() {}

    attach_id on_attached(monitor& mon, port_type& port, provider_hint hint) override {
	auto id = BaseService::on_attached(mon, port, hint);
	for (auto& sp: ext_delegate.services()) {
	    sp->on_attached(mon, port, hint);
	}
	return id;
    }

    detach_id on_detached(monitor& mon, port_type& port, provider_hint hint) override {
	auto id = BaseService::on_detached(mon, port, hint);
	for (auto& sp: ext_delegate.services()) {
	    sp->on_detached(mon, port, hint);
	}
	return id;
    }

    void on_message(monitor& mon, port_type& port, msgbuf_type& msg) override {
	BaseService::on_message(mon, port, msg);
	for (auto& sp: ext_delegate.services()) {
	    sp->on_message(mon, port, msg);
	}
    }

    void on_disconnected(monitor& mon, port_type& port, io_result& res) override {
	BaseService::on_disconnected(mon, port, res);
	for (auto& sp: ext_delegate.services()) {
	    sp->on_disconnected(mon, port, res);
	}
    }

    void on_error(monitor& mon, port_type& port, io_result& res) override {
	BaseService::on_error(mon, port, res);
	for (auto& sp: ext_delegate.services()) {
	    sp->on_error(mon, port, res);
	}
    }
};


} // c7::event::ext


#endif // c7event/ext/delegate.hpp
