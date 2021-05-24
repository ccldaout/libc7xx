/*
 * c7event/fsm.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_FSM_HPP_LOADED__
#define C7_EVENT_FSM_HPP_LOADED__
#include <c7common.hpp>


#include <c7fd.hpp>
#include <c7fsm.hpp>
#include <c7event/monitor.hpp>
#include <c7event/service.hpp>
#include <queue>


namespace c7::event {


class fsm_provider: public provider_interface {
public:
    using callback_t = c7::fsm::driver<void>::callback_t;
    using callback_id_t = c7::fsm::driver<void>::callback_id_t;
    using event_t = c7::fsm::driver<void>::event_t;

    static const char * const subscribe_key;

    fsm_provider(int evfd, c7::fsm::driver<void>&& fsm);
    int fd() override { return evfd_; };
    void on_event(monitor&, int, uint32_t) override;

    static result<> subscribe(monitor& mon, c7::fsm::driver<void>&& fsm,
			      const std::string& key = subscribe_key);
    static result<> subscribe(c7::fsm::driver<void>&& fsm,
			      const std::string& key = subscribe_key) {
	return subscribe(default_event_monitor(), std::move(fsm), key);
    }

    void link_callback(callback_id_t id, callback_t cb) { fsm_.link_callback(id, cb); }
    void unlink_callback(callback_id_t id) { fsm_.unlink_callback(id); }
    void start() { fsm_.start(); }
    void reset() { fsm_.reset(); }
    void commit(event_t event);

private:
    c7::fd evfd_;
    c7::fsm::driver<void> fsm_;
    c7::thread::mutex lock_;
    std::queue<event_t> evque_;
};


template <typename BaseService>
class fsm_service: public BaseService {
public:
    using port_type = typename BaseService::port_type;
    using msgbuf_type = typename BaseService::msgbuf_type;

    fsm_service() = default;

private:
    using callback_t = c7::fsm::driver<void>::callback_t;
    using callback_id_t = c7::fsm::driver<void>::callback_id_t;
    using event_t = c7::fsm::driver<void>::event_t;

protected:
    class bridge {
    public:
	~bridge() {
	    for (auto id: linked_cb_) {
		provider_->unlink_callback(id);
	    }
	}
	bridge() = default;
	bridge(const bridge&) = delete;
	bridge& operator=(const bridge&) = delete;

	result<> setup(monitor& mon,
		       const std::string& key = fsm_provider::subscribe_key);

	void link_callback(callback_id_t id, callback_t cb) {
	    provider_->link_callback(id, std::move(cb));
	    linked_cb_.push_back(id);
	}
	void unlink_callback(callback_id_t id) {
	    provider_->unlink_callback(id);
	}
	void start() { provider_->start(); }
	void reset() { provider_->reset(); }
	void commit(event_t event) { provider_->commit(event); }

    private:
	std::shared_ptr<fsm_provider> provider_;
	std::vector<callback_id_t> linked_cb_;
    };

    attach_id on_attached(monitor& mon, port_type& port, provider_hint hint) override {
	ext_fsm.setup(mon);
 	return BaseService::on_attached(mon, port, hint);
    }

    bridge ext_fsm;
};

template <typename BaseService>
result<>
fsm_service<BaseService>::bridge::setup(monitor& mon, const std::string& key)
{
    auto res = mon.find<fsm_provider>(fsm_provider::subscribe_key);
    if (!res) {
	return std::move(res);
    }
    provider_ = std::move(res.value());
    return c7result_ok();
}


} // c7::event


#endif // c7event/fsm.hpp
