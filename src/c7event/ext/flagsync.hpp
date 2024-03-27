/*
 * c7event/ext/flagsync.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_EXT_FLAGSYNC_HPP_LOADED_
#define C7_EVENT_EXT_FLAGSYNC_HPP_LOADED_
#include <c7common.hpp>


#include <c7event/monitor.hpp>
#include <c7event/service.hpp>
#include <c7fd.hpp>
#include <queue>


namespace c7::event::ext {


// provider

class flagsync_provider: public provider_interface {
public:
    using flags_t = uint32_t;
    using callback_t = std::function<void(flags_t&)>;
    using callback_id_t = uint64_t;

    static const char * const manage_key;

    ~flagsync_provider() = default;

    explicit flagsync_provider(int evfd): evfd_(evfd) {}
    int fd() override { return evfd_; };
    void on_event(monitor&, int, uint32_t) override;

    static result<> manage(monitor& mon);
    static result<> manage() { return manage(default_event_monitor()); }

    callback_id_t assign(std::weak_ptr<void> owner_wp,
			 flags_t require_flags, callback_t callback);
    void unassign(callback_id_t);
    void update(flags_t on, flags_t off);

private:
    struct callback_info {
	flags_t req_flags;
	std::weak_ptr<void> owner_wp;
	callback_id_t id;
	callback_t callback;
    };

    static std::atomic<uint64_t> id_counter_;

    c7::fd evfd_;
    c7::thread::mutex lock_que_;
    c7::thread::mutex lock_cbs_;
    std::queue<uint64_t> que_;		// uint64_t: on<<32|off
    std::vector<callback_info> callbacks_;
    flags_t flags_;
};


// service extention

template <typename BaseService>
class flagsync_service: public BaseService {
public:
    using port_type = typename BaseService::port_type;
    using msgbuf_type = typename BaseService::msgbuf_type;

    flagsync_service(): ext_flagsync(*this) {}

private:
    using callback_t = flagsync_provider::callback_t;
    using flags_t = flagsync_provider::flags_t;
    using callback_id_t = flagsync_provider::flags_t;

protected:
    class bridge {
    public:
	bridge(flagsync_service& owner): owner_(owner) {}
	bridge(const bridge&) = delete;
	bridge& operator=(const bridge&) = delete;

	result<> setup(monitor& mon);

	callback_id_t assign(flags_t require_flags, callback_t callback) {
	    auto id = provider_->assign(owner_.shared_from_this(), require_flags, std::move(callback));
	    return id;
	}

	template <typename T>
	callback_id_t assign(flags_t require_flags, void (T::*cbp)(flags_t&)) {
	    auto owner_p = &owner_;
	    callback_t callback = [owner_p, cbp](flags_t& flags) {
		(static_cast<T*>(owner_p)->*cbp)(flags);
	    };
	    auto id = provider_->assign(owner_.shared_from_this(), require_flags, std::move(callback));
	    return id;
	}

	void unassign(callback_id_t id) {
	    provider_->unassign(id);
	}

	void update(flags_t on, flags_t off) {
	    provider_->update(on, off);
	}

    private:
	flagsync_service& owner_;
	std::shared_ptr<flagsync_provider> provider_;
    };

    attach_id on_attached(monitor& mon, port_type& port, provider_hint hint) override {
	ext_flagsync.setup(mon);
	return BaseService::on_attached(mon, port, hint);
    }

    bridge ext_flagsync;
};

template <typename BaseService>
result<>
flagsync_service<BaseService>::bridge::setup(monitor& mon)
{
    auto res = mon.find<flagsync_provider>(flagsync_provider::manage_key);
    if (!res) {
	return std::move(res);
    }
    provider_ = std::move(res.value());
    return c7result_ok();
}


} // c7::event::ext


#endif // c7event/ext/flagsync.hpp
