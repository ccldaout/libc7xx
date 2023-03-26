/*
 * c7event/ext/forwarder.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_EXT_FORWARDER_HPP_LOADED__
#define C7_EVENT_EXT_FORWARDER_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/service.hpp>
#include <list>
#include <unordered_set>


namespace c7::event::ext {


template <typename>
class forwarder;


template <typename, typename>
class forwarder_broker;


template <typename Msgbuf, typename Port = socket_port>
class forwarder_proxy {
public:
    using broker = forwarder_broker<Msgbuf, Port>;
    using callback_t = void(monitor&, Port&, io_result&, Msgbuf&);

    void set_callback(const std::function<callback_t>& cb) {
	auto unlock = mutex_.lock();
	callback_ = cb;
    }
    void subscribe(const std::vector<int32_t> interests) {
	auto unlock = mutex_.lock();
	interests_.insert(interests.begin(), interests.end());
    }
    void unsubscribe(const std::vector<int32_t> interests) {
	auto unlock = mutex_.lock();
	for (auto e: interests) {
	    interests_.erase(e);
	}
    }
    void unsubscribe() {
	auto unlock = mutex_.lock();
	interests_.clear();
    }

private:
    template <typename, typename>
    friend class forwarder_broker;

    std::unordered_set<int32_t> interests_;
    std::function<callback_t> callback_;
    c7::thread::mutex mutex_;

    void call(monitor& mon, Port& port, io_result& res, Msgbuf& msg) {
	auto unlock = mutex_.lock();
	auto callback = callback_;
	if (interests_.empty() || !callback) {
	    return;
	}
	auto match = (interests_.find(get_event(msg)) != interests_.end());
	unlock();
	if (match || !res) {
	    callback(mon, port, res, msg);
	}
    }
};


template <typename Msgbuf, typename Port = socket_port>
class forwarder_broker {
private:
    using proxy = forwarder_proxy<Msgbuf, Port>;

    template <typename>
    friend class forwarder;

    c7::thread::mutex mutex_;
    std::list<std::weak_ptr<proxy>> proxies_;

    void forward(monitor& mon, Port& port, io_result& res, Msgbuf& msg) {
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

public:
    std::shared_ptr<proxy> operator()() {
	auto unlock = mutex_.lock();
	auto p = std::make_shared<proxy>();
	proxies_.push_back(p);
	return p;
    }
};


template <typename BaseService>
class forwarder: public BaseService {
public:
    using port_type = typename BaseService::port_type;
    using msgbuf_type = typename BaseService::msgbuf_type;

    forwarder_broker<msgbuf_type, port_type> ext_forwarder;

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


} // c7::event::ext


#endif // c7event/ext/forwarder.hpp
