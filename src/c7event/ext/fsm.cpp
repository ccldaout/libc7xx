/*
 * c7event/fsm.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7event/ext/fsm.hpp>
#include <sys/eventfd.h>


namespace c7::event::ext {


fsm_provider::fsm_provider(int evfd, c7::fsm::driver<void>&& fsm):
    evfd_(evfd), fsm_(std::move(fsm))
{
}

void fsm_provider::on_event(monitor&, int, uint32_t)
{
    uint64_t n;
    auto io_res = evfd_.read_n(&n);
    if (io_res) {
	for (uint64_t i = 0; i < n; i++) {
	    auto unlock = lock_.lock();
	    event_t event = evque_.front();
	    evque_.pop();
	    unlock();
	    fsm_.transit(event);
	}
    } else {
	// TODO: error (mlog)
    }
}

// static member
result<> fsm_provider::manage(monitor& mon, c7::fsm::driver<void>&& fsm, const std::string& key)
{
    int evfd_ = ::eventfd(0, EFD_CLOEXEC);
    if (evfd_ == C7_SYSERR) {
	return c7result_err(errno, "eventfd() failed");
    }
    auto drv = std::make_shared<fsm_provider>(evfd_, std::move(fsm));
    return mon.manage(key, std::move(drv));
}

void fsm_provider::commit(event_t event)
{
    auto unlock = lock_.lock();
    evque_.push(event);
    unlock();
    uint64_t counter = 1;		// 'counter' mean number of event queued here.
    evfd_.write_n(&counter);
}

const char * const fsm_provider::manage_key = "c7::event::ext::fsm_provider";


} // c7::event::ext
