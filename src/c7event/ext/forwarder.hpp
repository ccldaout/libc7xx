/*
 * c7event/forwarder.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_FORWARDER_HPP_LOADED__
#define C7_EVENT_FORWARDER_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/service.hpp>
#include <list>
#include <unordered_set>


namespace c7::event {


template <typename BaseService>
class forwarder: public BaseService {
public:
    using port_type = typename BaseService::port_type;
    using msgbuf_type = typename BaseService::msgbuf_type;
    using monitor   = c7::event::monitor;

    class broker {
    private:
	class proxy {
	public:
	    using callback_t = void(monitor&, port_type&, io_result&, msgbuf_type&);
	    explicit proxy(broker& b): broker_(b) {}
	    void set_callback(const std::function<callback_t>& cb) {
		callback_ = cb;
	    }
	    void subscribe(const std::vector<int32_t> interests) {
		interests_.insert(interests.begin(), interests.end());
	    }
	    void unsubscribe(const std::vector<int32_t> interests) {
		for (auto e: interests) {
		    interests_.erase(e);
		}
	    }
	    void unsubscribe() {
		interests_.clear();
	    }
	    ~proxy() { broker_.uncontract(this); }

	private:
	    friend class broker;
	    
	    std::unordered_set<int32_t> interests_;
	    std::function<callback_t> callback_;
	    broker& broker_;

	    void call(monitor& mon, port_type& port, io_result& res, msgbuf_type& msg) {
		if (interests_.find(get_event(msg)) != interests_.end() || !res) {
		    callback_(mon, port, res, msg);
		}
	    }
	};

	friend class forwarder;

	c7::thread::mutex mutex_;
	std::list<std::weak_ptr<proxy>> proxies_;

	void forward(monitor& mon, port_type& port, io_result& res, msgbuf_type& msg) {
	    std::vector<std::shared_ptr<proxy>> spv;
	    auto unlock = mutex_.lock();
	    for (auto it = proxies_.begin(); it != proxies_.end();) {
		auto sp = (*it).lock();
		if (!sp) {
		    it = proxies_.erase(it);
		} else {
		    ++it;
		    spv.push_back(std::move(sp));
		}
	    }
	    unlock();
	    for (auto sp: spv) {
		sp->call(mon, port, res, msg);
	    }
	}

	void uncontract(proxy *p) {
	    auto unlock = mutex_.lock();
	    for (auto it = proxies_.begin(); it != proxies_.end();) {
		auto sp = (*it).lock();
		if (!sp || sp.get() == p) {
		    it = proxies_.erase(it);
		} else {
		    ++it;
		}
	    }
	}

    public:
	std::shared_ptr<proxy> operator()() {
	    auto unlock = mutex_.lock();
	    auto p = std::make_shared<proxy>(*this);
	    proxies_.push_back(p);
	    return p;
	}
    };

    broker ext_forwarder;

    forwarder() {}

    void on_message(monitor& mon, port_type& port, msgbuf_type& msg) override {
	BaseService::on_message(mon, port, msg);
	auto res = c7::io_result::ok();
	ext_forwarder.forward(mon, port, res, msg);
    }

    void on_disconnected(monitor& mon, port_type& port, io_result& res) override {
	BaseService::on_disconnected(mon, port, res);
	auto msg = msgbuf_type{};
	ext_forwarder.forward(mon, port, res, msg);
    }

    void on_error(monitor& mon, port_type& port, io_result& res) override {
	BaseService::on_error(mon, port, res);
	auto msg = msgbuf_type{};
	ext_forwarder.forward(mon, port, res, msg);
    }

private:
};


} // c7::event


#endif // c7event/ext/forwarder.hpp
