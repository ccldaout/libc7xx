/*
 * c7fsm.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_FSM_HPP_LOADED__
#define C7_FSM_HPP_LOADED__
#include <c7common.hpp>


#include <c7result.hpp>
#include <c7thread.hpp>
#include <queue>
#include <unordered_map>
#include <unordered_set>


template <>
struct std::hash<std::pair<int, int>> {
    size_t operator()(const std::pair<int, int>& k) const {
	return std::hash<size_t>()((static_cast<uint64_t>(k.first)<<32)
				   |static_cast<uint64_t>(k.second));
    }
};


namespace c7::fsm {


class driver_base {
public:
    typedef int state_t;
    typedef int event_t;
    typedef int callback_id_t;
    typedef std::function<void(void*, state_t, state_t)> callback_t;
    struct transition {
	state_t cur_state;
	event_t event;
	state_t next_state;
	callback_id_t callback_id;
    };

    driver_base();
    driver_base(const driver_base&) = delete;
    driver_base& operator=(const driver_base&) = delete;
    driver_base(driver_base&&);
    driver_base& operator=(driver_base&&);

    // construct phase
    result<> define_combined(event_t combined_event, std::vector<event_t> partial_events);
    result<> add_transition(state_t cur, event_t ev, state_t next, callback_t);
    result<> add_transition(state_t cur, event_t ev, state_t next, callback_id_t);
    result<> add_transition(const std::vector<transition>&);
    result<> add_transition(const transition *tab, size_t n) {
	return add_transition(std::vector<transition>(tab, &tab[n]));
    }
    template <size_t N> result<> add_transition(const transition (*tab)[N]) {
	return add_transition(&(*tab)[0], N);
    }
    void link_callback(int callback_id, callback_t callback);
    void unlink_callback(int callback_id);
    void initial_state(state_t s) { initial_state(std::vector({s})); }
    void initial_state(const std::vector<state_t>&);
	     
    // running phase
    result<> start();
    result<> transit(event_t ev, void *ctx);		// [CAUTION] callback is called.
    std::vector<state_t> current_state() { return currents_; };
    void reset();

    uint64_t id() const { return id_; }
    void dump() const;

private:
    typedef std::pair<state_t, event_t> table_key_t;
    typedef std::pair<state_t, callback_id_t> table_val_t;

    uint64_t id_;
    std::unique_ptr<c7::thread::mutex> lock_;
    std::vector<state_t> initials_;
    std::vector<state_t> currents_;
    std::unordered_map<table_key_t, table_val_t> table_;
    std::unordered_map<callback_id_t, callback_t> callbacks_;
    std::unordered_set<event_t> def_partials_;
    std::unordered_set<event_t> ena_partials_;
    std::unordered_map<event_t, std::vector<event_t>> combined_;

    bool is_partial(event_t);
    bool try_get_combined(event_t&);
    void set_cur(state_t s);
    bool step(state_t& cur, event_t ev, void *ctx);
};


template <typename T>
class driver: public driver_base {
public:
    typedef std::function<void(T&, state_t, state_t)> callback_t;

    // construct phase
    using driver_base::add_transition;

    result<> add_transition(state_t cur, event_t ev, state_t next, callback_t callback) {
	return driver_base::add_transition(cur, ev, next,
					   [cb=std::move(callback)](auto *p, auto c, auto n) {
					       cb(*static_cast<T*>(p), c, n);
					   });
    }

    void link_callback(int callback_id, callback_t callback) {
	driver_base::link_callback(callback_id,
				   [cb=std::move(callback)](auto *p, auto c, auto n) {
				       cb(*static_cast<T*>(p), c, n);
				   });
    }
	     
    // running phase
    result<> transit(event_t ev, T& ctx) {
	return driver_base::transit(ev, &ctx);
    }
};


template <>
class driver<void>: public driver_base {
public:
    typedef std::function<void(state_t, state_t)> callback_t;

    // construct phase
    using driver_base::add_transition;

    result<> add_transition(state_t cur, event_t ev, state_t next, callback_t callback) {
	return driver_base::add_transition(cur, ev, next,
					   [cb=std::move(callback)](auto *, auto c, auto n) {
					       cb(c, n);
					   });
    }

    void link_callback(int callback_id, callback_t callback) {
	driver_base::link_callback(callback_id,
				   [cb=std::move(callback)](auto *, auto c, auto n) {
				       cb(c, n);
				   });
    }
	     
    // running phase
    result<> transit(event_t ev) {
	return driver_base::transit(ev, nullptr);
    }
};


class machine {
public:
    using event_t = driver_base::event_t;
    using state_t = driver_base::state_t;

    machine(c7::fsm::driver_base&& driver): driver_(std::move(driver)) {}

    result<> loop(event_t trigger, std::vector<state_t> terminals, void *ctx);

    template <typename T>
    result<> loop(event_t trigger, std::vector<state_t> terminals, T& ctx) {
	return loop(trigger, terminals, &ctx);
    }

    void commit(event_t tigger);

private:
    c7::thread::condvar cv_;
    std::queue<event_t> evque_;
    driver_base driver_;
};


} // namespace c7::fsm


#endif // c7fsm.hpp
