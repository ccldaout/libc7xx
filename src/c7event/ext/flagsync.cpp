/*
 * c7event/flagsync.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7event/ext/flagsync.hpp>
#include <sys/eventfd.h>


namespace c7::event::ext {


void flagsync_provider::on_event(monitor&, int, uint32_t)
{
    uint64_t n;
    auto io_res = evfd_.read_n(&n);
    if (!io_res) {
	// TODO: error (mlog)
	return;
    }

    for (uint64_t i = 0; i < n; i++) {
	auto unlock_que = lock_que_.lock();
	uint64_t data = que_.front();
	que_.pop();
	unlock_que();

	flags_t cur = flags_;
	flags_t on = data >> 32;
	flags_t off = (data << 32) >> 32;
	flags_ |= on;
	flags_ &= ~off;
	if (flags_ == cur) {
	    continue;
	}

	for (;;) {
	    // [MEMO] The callbacks_ vector can be updated in callback running,
	    //        it's the reason why we must find from head again. 
	    auto unlock_cbs = lock_cbs_.lock();
	    auto it = std::find_if(callbacks_.begin(), callbacks_.end(),
				   [flags=flags_](auto& cur){
				       auto req_flags = cur.req_flags;
				       return (flags & req_flags) == req_flags;
				   });
	    if (it == callbacks_.end()) {
		break;
	    }
	    auto [req_flags, owner_wp, id, callback] = *it;
	    auto sp = owner_wp.lock();
	    if (sp == nullptr) {
		callbacks_.erase(it);
	    }
	    unlock_cbs();

	    if (sp) {
		flags_ &= ~req_flags;
		callback(flags_);
	    }
	}
    }
}

// static member
result<> flagsync_provider::manage(monitor& mon)
{
    int evfd_ = ::eventfd(0, EFD_CLOEXEC);
    if (evfd_ == C7_SYSERR) {
	return c7result_err(errno, "eventfd() failed");
    }
    auto prv = std::make_shared<flagsync_provider>(evfd_);
    return mon.manage(manage_key, std::move(prv));
}

auto flagsync_provider::assign(std::weak_ptr<void> owner_wp,
			       flags_t req_flags, callback_t callback)
    -> callback_id_t 
{
    auto unlock_cbs = lock_cbs_.lock();
    auto id = ++id_counter_;
    callback_info cbi { req_flags, std::move(owner_wp), id, std::move(callback) };
    callbacks_.emplace_back(std::move(cbi));
    return id;
}

void flagsync_provider::unassign(callback_id_t id)
{
    auto unlock_cbs = lock_cbs_.lock();
    std::remove_if(callbacks_.begin(), callbacks_.end(),
		   [id](auto& it) { return it.id == id; });
}

void flagsync_provider::update(flags_t on, flags_t off)
{
    auto unlock_que = lock_que_.lock();
    uint64_t data = ((uint64_t)on) << 32 | off;
    que_.push(data);
    unlock_que();

    uint64_t counter = 1;		// 'counter' mean number of event queued here.
    evfd_.write_n(&counter);
}

std::atomic<uint64_t> flagsync_provider::id_counter_;
const char * const flagsync_provider::manage_key = "c7::event::ext::syncflag_provider";


} // c7::event::ext
