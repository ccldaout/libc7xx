/*
 * c7fsm.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7fsm.hpp>


namespace c7::fsm {


static std::atomic<uint64_t> id_counter;


driver_base::driver_base():
    id_(++id_counter), table_(128), callbacks_(128),
    def_partials_(32), ena_partials_(32), combined_(16)
{
}

driver_base::driver_base(driver_base&& o):
    id_(o.id_),
    lock_(std::move(o.lock_)),
    initials_(std::move(o.initials_)),
    currents_(std::move(o.currents_)),
    table_(std::move(o.table_)),
    callbacks_(std::move(o.callbacks_)),
    def_partials_(std::move(o.def_partials_)),
    ena_partials_(std::move(o.ena_partials_)),
    combined_(std::move(o.combined_))
{
}

auto driver_base::operator=(driver_base&& o) -> driver_base&
{
    if (this != &o) {
	id_           = o.id_;
	lock_         = std::move(o.lock_);
	initials_     = std::move(o.initials_);
	currents_     = std::move(o.currents_);
	table_        = std::move(o.table_);
	callbacks_    = std::move(o.callbacks_);
	def_partials_ = std::move(o.def_partials_);
	ena_partials_ = std::move(o.ena_partials_);
	combined_     = std::move(o.combined_);
    }
    return *this;
}

bool driver_base::is_partial(event_t ev)
{
    auto it = def_partials_.find(ev);
    return (it != def_partials_.end());
}

bool driver_base::try_get_combined(event_t& ev)
{
    ena_partials_.insert(ev);
    for (auto& [com_ev, partials]: combined_) {
	bool found = true;
	for (auto p_ev: partials) {
	    if (ena_partials_.find(p_ev) == ena_partials_.end()) {
		found = false;
		break;
	    }
	}
	if (found) {
	    ev = com_ev;
	    for (auto p_ev: partials) {
		ena_partials_.erase(p_ev);
	    }
	    return true;
	}
    }
    return false;
}

bool driver_base::step(state_t& cur, event_t ev, void *ctx)
{
    auto unlock = lock_.lock();

    auto it = table_.find(table_key_t(cur, ev));
    if (it == table_.end()) {
	return false;
    }

    auto& [next_state, callback_id] = (*it).second;
    auto cbit = callbacks_.find(callback_id);

    auto cur_state = cur;
    cur = next_state;
    unlock();
    if (cbit != callbacks_.end()) {
	(*cbit).second(ctx, cur_state, next_state);
    }
    return true;

}

result<> driver_base::define_combined(event_t combined_event, std::vector<event_t> partial_events)
{
    for (auto ev: partial_events) {
	auto [_, res] = def_partials_.insert(ev);
	if (!res) {
	    return c7result_err(EEXIST, "event:%{} is already exist in partials.", ev);
	}
    }
    auto [_, res] = combined_.try_emplace(combined_event, std::move(partial_events));
    if (!res) {
	return c7result_err(EEXIST, "event:%{} is already exist in combineds.", combined_event);
    }
    return c7result_ok();
}

result<> driver_base::add_transition(state_t cur, event_t ev, state_t next, callback_t callback)
{
    static std::atomic<uint64_t> id { 1UL<<32 };
    ++id;
    if (auto res = add_transition(cur, ev, next, id); !res) {
	return res;
    }
    link_callback(id, std::move(callback));
    return c7result_ok();
}
	     
result<> driver_base::add_transition(state_t cur, event_t ev, state_t next, callback_id_t callback_id)
{
    auto key = table_key_t(cur, ev);
    auto val = table_val_t(next, callback_id);
    auto [_, res] = table_.try_emplace(key, std::move(val));
    if (res) {
	return c7result_ok();
    } else {
	return c7result_err(EEXIST, "<state:%{}, event:%{}> is already exist.", cur, ev);
    }
}
	     
result<> driver_base::add_transition(const std::vector<transition>& transitions)
{
    for (auto& [cur, ev, next, callback_id]: transitions) {
	if (auto res = add_transition(cur, ev, next, callback_id); !res) {
	    return res;
	}
    }
    return c7result_ok();
}

void driver_base::link_callback(int callback_id, callback_t callback)
{
    callbacks_.insert_or_assign(callback_id, std::move(callback));
}

void driver_base::unlink_callback(int callback_id)
{
    callbacks_.erase(callback_id);
}

void driver_base::initial_state(const std::vector<state_t>& inits)
{
    initials_ = inits;
}

result<> driver_base::start()
{
    // all combined event don't appear in def_partials_
    for (auto& [com_ev, _]: combined_) {
	if (is_partial(com_ev)) {
	    return c7result_err(EINVAL, "combined event:%{} appear in partials", com_ev);
	}
    }

    // all event in table_ don't appear in def_partials_
    for (auto& [key, val]: table_) {
	if (is_partial(key.second)) {
	    return c7result_err(EINVAL, "event:%{} in transition table appear in partials",
				key.second);
	}
    }

    reset();
    return c7result_ok();
}

result<> driver_base::transit(event_t ev, void *ctx)
{
    if (is_partial(ev)) {
	if (!try_get_combined(ev)) {
	    return c7result_ok();
	}
    }

    bool applied = false;
    for (auto& cur: currents_) {
	if (step(cur, ev, ctx)) {
	    applied = true;
	}
    }
    if (applied) {
	return c7result_ok();
    } else {
	return c7result_err(ENOENT, "state:%{} and event:%{} has no transition.");
    }
}

void driver_base::reset()
{
    currents_ = initials_;
    ena_partials_.clear();
}

void driver_base::dump() const
{
    
    c7::p__("currents_:");
    for (auto cur: currents_) {
	c7::p__(" %{}", cur);
    }
    c7::p_("");
    for (auto& [key, val]: table_) {
	c7::p_("  entry: state:%{} ev:%{} -> next:%{}", key.first, key.second, val.first);
    }
}


result<> machine::loop(event_t event, std::vector<state_t> terminals, void *ctx)
{
    if (auto res = driver_.start(); !res) {
	return res;
    }
    for (;;) {
	driver_.transit(event, ctx);
	if (driver_.current_state() == terminals) {
	    return c7result_ok();
	}
	{
	    auto unlock_ = cv_.lock();
	    cv_.wait_while([this](){ return evque_.empty(); });
	    event = evque_.front();
	    evque_.pop();
	}
    }
}

void machine::commit(event_t event)
{
    cv_.lock_notify([this, event]() { evque_.push(event); });
}


} // namespace c7::fsm
