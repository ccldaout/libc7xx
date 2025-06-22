/*
 * c7event/submit.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7event/submit.hpp>
#include <sys/eventfd.h>


namespace c7::event {


c7::result<std::shared_ptr<submit_provider>>
submit_provider::make()
{
    auto sp = std::shared_ptr<submit_provider>(new submit_provider());
    if (auto res = sp->init(); !res) {
	return res.as_error();
    }
    return c7result_ok(sp);
}


c7::result<std::shared_ptr<submit_provider>>
submit_provider::make_and_manage()
{
    return make_and_manage(c7::event::default_event_monitor());
}


c7::result<std::shared_ptr<submit_provider>>
submit_provider::make_and_manage(c7::event::monitor& mon)
{
    static c7::thread::mutex m;
    auto unlock = m.lock();

    if (auto res = mon.find<submit_provider>(manage_key); res) {
	return res;
    }
    if (auto res = make(); !res) {
	return res;
    } else {
	auto sp = std::move(res.value());
	if (auto res = mon.manage(manage_key, sp); !res) {
	    return res.as_error();
	}
	return c7result_ok(sp);
    }
}


c7::result<>
submit_provider::init()
{
    int fd = ::eventfd(0, EFD_CLOEXEC);
    if (fd == C7_SYSERR) {
	return c7result_err(errno, "Failed to create eventfd.");
    }
    evfd_ = c7::fd(fd);
    return c7result_ok();
}


void
submit_provider::on_event(c7::event::monitor&, int prvfd, uint32_t events)
{
    uint64_t submit_count;
    if (auto io_res = evfd_.read_n(&submit_count); !io_res) {
	auto res = c7result_err(std::move(io_res.get_result()), "Failed to read eventfd.");
	on_error(res);
	return;
    }
    for (uint64_t i = 0; i < submit_count; i++) {
	if (auto q_res = callbacks_.get(0); !q_res) {
	    auto res = c7result_err(std::move(q_res.as_error()),
				    "rest submit count:%{} (from eventfd)", submit_count - i);
	    on_error(res);
	} else {
	    q_res.value()();
	}
    }
}


c7::result<>
submit_provider::submit(std::function<void()>&& f)
{
    if (auto res = callbacks_.put(std::move(f)); !res) {
	return res;
    }
    uint64_t submit_count = 1;
    if (auto io_res = evfd_.write_n(&submit_count); !io_res) {
	return c7result_err(std::move(io_res.get_result()), "Failed to write eventfd.");
    }
    return c7result_ok();
}


} // namespae c7::event

