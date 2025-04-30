/*
 * c7thread/msgbox.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_THREAD_MSGBOX_HPP_LOADED_
#define C7_THREAD_MSGBOX_HPP_LOADED_
#include <c7common.hpp>


#include <unistd.h>
#include <atomic>
#include <list>
#include <memory>
#include <unordered_map>
#include <c7hash.hpp>
#include <c7result.hpp>
#include <c7thread/condvar.hpp>
#include <c7utils/time.hpp>


namespace c7::thread {


namespace msgbox_impl {

class counter {
public:
    counter(): counter_(static_cast<uint64_t>(getpid()) << 40) {}
    counter(const counter&) = delete;
    counter& operator=(const counter&) = delete;
    counter(counter&&) = delete;
    counter& operator=(counter&&) = delete;

    uint64_t operator()() { return ++counter_; }

private:
    std::atomic<uint64_t> counter_;
};

extern counter tray_id;

}


template <typename Msg>
class msgbox {
private:
    class impl;
    std::shared_ptr<impl> pimpl_;

public:
    using u64_key = c7::simple_wrap<uint64_t, msgbox<Msg>>;

    class tray_key {
    public:
	~tray_key() {
	    if (pimpl_) {
		pimpl_->unref(key_);
	    }
	}
	tray_key() = default;
	tray_key(u64_key key, std::shared_ptr<impl> pimpl):
	    key_(key), pimpl_(std::move(pimpl)) {
	    pimpl_->ref(key);
	}
	tray_key(const tray_key& o):
	    key_(o.key_), pimpl_(o.pimpl_) {
	    pimpl_->ref(key_);
	}
	tray_key& operator=(const tray_key& o) {
	    if (this != &o) {
		if (pimpl_) {
		    pimpl_->unref(key_);
		}
		key_   = o.key_;
		pimpl_ = o.pimpl_;
		pimpl_->ref(key_);
	    }
	    return *this;
	}
	tray_key(tray_key&& o):
	    key_(o.key_), pimpl_(std::move(o.pimpl_)) {
	}
	tray_key& operator=(tray_key&& o) {
	    if (this != &o) {
		if (pimpl_) {
		    pimpl_->unref(key_);
		}
		key_   = o.key_;
		pimpl_ = std::move(o.pimpl_);
	    }
	    return *this;
	}

	u64_key operator()() const {
	    return key_;
	}

	void print(std::ostream& out, const std::string&) const {
	    c7::format(out, "msgbox::tray_key<%{}>", key_);
	}

    private:
	u64_key key_;
	std::shared_ptr<impl> pimpl_;
    };

    msgbox(): pimpl_(std::make_shared<impl>()) {}

    tray_key reserve() {
	return pimpl_->reserve();
    }

    c7::result<std::list<Msg>> wait(const tray_key& tray,
				    const ::timespec* timeout_abs = nullptr) {
	return pimpl_->wait(tray, timeout_abs);
    }

    auto wait(const tray_key& tray, c7::usec_t duration_us) {
	auto timeout_abs = c7::mktimespec(duration_us);
	return pimpl_->wait(tray, timeout_abs);
    }

    c7::result<> close(tray_key tray_key) {
	return pimpl_->close(tray_key());
    }

    c7::result<> close(u64_key key) {
	return pimpl_->close(key);
    }

    c7::result<> put(u64_key key, const Msg& msg) {
	return pimpl_->put_internal(key, msg);
    }

    c7::result<> put(u64_key key, Msg&& msg) {
	return pimpl_->put_internal(key, std::move(msg));
    }

    size_t size() const {
	return pimpl_->size();
    }

private:
    class impl: public std::enable_shared_from_this<impl> {
    private:
	struct msg_tray {
	    std::list<Msg> msgs;
	    uint64_t rc = 0;
	};

	c7::thread::condvar cv_;
	std::unordered_map<u64_key, std::shared_ptr<msg_tray>> box_;

    public:
	tray_key reserve() {
	    u64_key key{msgbox_impl::tray_id()};
	    auto unlock = cv_.lock();
	    auto tray = std::make_shared<msg_tray>();
	    box_.emplace(key, tray);
	    unlock();
	    return tray_key(key, this->shared_from_this());
	}

	c7::result<std::list<Msg>> wait(const tray_key& key,
					const ::timespec* timeout_abs = nullptr) {
	    auto unlock = cv_.lock();
	    for (;;) {
		auto it = box_.find(key());
		if (it == box_.end()) {
		    return c7result_err(ENOENT, "msg tray for key:%{} is not found", key());
		}
		auto& tray = (*it).second;
		if (!tray->msgs.empty()) {
		    return c7result_ok(std::move(tray->msgs));
		}
		if (!cv_.wait(timeout_abs)) {
		    return c7result_err(EAGAIN, "timeout of waiting msg for key:%{}", key());
		}
	    }
	}

	c7::result<> ref(u64_key key) {
	    auto unlock = cv_.lock();
	    if (auto it = box_.find(key); it == box_.end()) {
		return c7result_err(ENOENT, "msg tray for key:%{} is not found", key);
	    } else {
		auto& tray = (*it).second;
		tray->rc++;
		return c7result_ok();
	    }
	}

	c7::result<> unref(u64_key key) {
	    auto unlock = cv_.lock();
	    if (auto it = box_.find(key); it == box_.end()) {
		return c7result_err(ENOENT, "msg tray for key:%{} is not found", key);
	    } else {
		auto& tray = (*it).second;
		tray->rc--;
		if (tray->rc == 0) {
		    box_.erase(key);
		    cv_.notify_all();
		}
		return c7result_ok();
	    }
	}

	c7::result<> close(u64_key key) {
	    auto unlock = cv_.lock();
	    if (box_.erase(key) == 0) {
		return c7result_err(ENOENT, "msg tray for key:%{} is not found", key);
	    }
	    cv_.notify_all();
	    return c7result_ok();
	}

	template <typename M>
	c7::result<> put_internal(u64_key key, M&& msg) {
	    auto unlock = cv_.lock();
	    auto it = box_.find(key);
	    if (it == box_.end()) {
		return c7result_err(ENOENT, "msg tray for key:%{} is not found", key);
	    }
	    auto& tray = (*it).second;
	    tray->msgs.push_back(std::forward<M>(msg));
	    cv_.notify_all();
	    return c7result_ok();
	}

	size_t size() const {
	    return box_.size();
	}

	auto lock() {
	    return cv_.lock();
	}
    };

};


} // namespace c7::thread


#endif // c7thread/msgbox.hpp
