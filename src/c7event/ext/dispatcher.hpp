/*
 * c7event/ext/dispatcher.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_EXT_DISPATCHER_HPP_LOADED__
#define C7_EVENT_EXT_DISPATCHER_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/service.hpp>
#include <c7typefunc.hpp>


namespace c7::event::ext {


namespace {

c7typefunc_define_has_member(enter_callback);
c7typefunc_define_has_member(exit_callback);

template <typename Msgbuf, typename Port, typename... Hooks>
struct dispatcher_hooks;

template <typename Msgbuf, typename Port, typename Hook, typename... Hooks>
struct dispatcher_hooks<Msgbuf, Port, Hook, Hooks...>:
    public Hook, public dispatcher_hooks<Msgbuf, Port, Hooks...> {
private:
    using base_type = dispatcher_hooks<Msgbuf, Port, Hooks...>;

public:
    void enter_callback(monitor& mon, Port& port, Msgbuf& msg) {
	if constexpr (has_enter_callback_v<Hook>) {
	    Hook::enter_callback(mon, port, msg);
	}
	base_type::enter_callback(mon, port, msg);
    }
    void exit_callback(monitor& mon, Port& port, Msgbuf& msg) {
	if constexpr (has_exit_callback_v<Hook>) {
	    Hook::exit_callback(mon, port, msg);
	}
	base_type::exit_callback(mon, port, msg);
    }
};

template <typename Msgbuf, typename Port>
struct dispatcher_hooks<Msgbuf, Port> {
    void enter_callback(monitor&, Port&, Msgbuf&) {}
    void exit_callback(monitor&, Port&, Msgbuf&) {}
};

}


// event dispatch extention
// ------------------------
//
// - dispatcher template require two typename parameters. First is target class which
//   hvae callback functions to be called by the dispathcer, and second is service base
//   class. It is service_interface in many case, but you intend to use other extenstion,
//   you shoud specify it as second.
//
//   [Example 1]
//
//      class my_service: public dispatcher<my_service, service_interface<iov_msgbuf>> {
//         ...
//
//   [Example 2]
//
//     my_service <- forwarder <- dispatcher <- fsm_service <- service_interface
//
//      class my_service:
//         public forwarder<dispatcher<my_service, fsm_service<service_interface<iov_msgbuf>>>> {
//         ...
//
// - dispatcher template require instance of c7::event::get_event() template function.
//
// - setup callbacks
//
//   [Basic]
//
//      class my_service: ... {
//      public:
//          ...
//          // You define dispatcher_setup as inline member function.
//          void dispatcher_setup() {
//              dispatcher_set(EventNumber, &my_service::handle_event);
//              ...
//          }
//
//          // You must define callback_default.
//          void callback_default(monitor& mon, port_type& port, const msgbuf_type& msg) override;
//
//          // Event handler has same signature with on_message.
//          void handle_event(monitor& mon, port_type& port, const msgbuf_type& msg);
//
//          ...
//
//      };
//
//   [Advanced]
//
//     You can omit setup codes in the dispatcher_setup by using setup_dispatcher.py if you
//     specify callbacks declaration as follow.
//
//      class my_service: ... {
//          ...
//          // You only declare dispatcher_setup.
//          void dispatcher_setup();
//
//          // You must define callback_default.
//          void callback_default(monitor& mon, port_type& port, const msgbuf_type& msg) override;
//
//          // [TYPE 1]
//          // callback function name is delived by event mnemonic with callback_ prefix.
//
//          void callback_EVENT_NAME(monitor& mon, port_type& port, const msgbuf_type& msg);
//
//          // [TYPE 2]
//          // you can assign one callback function to two or more events by specifying
//          // special comment //[[dispatcher:callback EVENT_RANGE ...] ahead of declaration.
//          // EVENT_RANGE is single event number or CLOSED interval represented by two event
//          // numbers with infix colon.
//
//          //[dispatcher:callback EVENT_NAME_1:EVENT_NAME_2 ...]
//          void callback_FAVORITE_NAME(monitor& mon, port_type& port, const msgbuf_type& msg);
//
//          // When multiple functions assign to one event number, one is selected by their
//          // priotity. [TYPE 1] is higher than [TYPE 2]. First assigned is higher than later
//          // assigned in same TYPE.
//          ...
//      };
//
//      void my_service::dispatcher_setup()
//      {
//          // The dispatcher_setup is defined out of class declaration and it include two
//          // special comments //[dispatcher:setup begin] and //[dispatcher:setup end].
//          // The setup_dispatcher.py replace the text surrounded by them with setup codes
//          // delived from class declaration.
//
//          //[dispatcher:setup begin]
//          //[dispatcher:setup end]
//      }
//
template <typename DelivedService, typename BaseService, typename... Hooks>
class dispatcher:
	public BaseService,
	public dispatcher_hooks<typename BaseService::msgbuf_type,
				typename BaseService::port_type,
				Hooks...> {
public:
    using port_type = typename BaseService::port_type;
    using msgbuf_type = typename BaseService::msgbuf_type;
    using memfunc_ptr = void (DelivedService::*)(monitor&, port_type&, const msgbuf_type&);
    using hooks_base = dispatcher_hooks<msgbuf_type, port_type, Hooks...>;

    dispatcher() {
	static_cast<DelivedService*>(this)->dispatcher_setup();
    }

    void on_message(monitor& mon, port_type& port, msgbuf_type& msg) override {
	BaseService::on_message(mon, port, msg);
	auto& evmap = dispatcher_.event_map;
	if (auto it = evmap.find(get_event(msg)); it != evmap.end()) {
	    auto [_, vec_index] = (*it).second;
	    auto mfp = dispatcher_.callback_vec[vec_index];
	    hooks_base::enter_callback(mon, port, msg);
	    (static_cast<DelivedService*>(this)->*mfp)(mon, port, msg);
	    hooks_base::exit_callback(mon, port, msg);
	} else {
	    callback_default(mon, port, msg);
	}
    }

    virtual void callback_default(monitor& mon, port_type& port, const msgbuf_type& msg) {}

protected:
    // high priority assign:
    void dispatcher_set(int32_t ev, memfunc_ptr mfp) {
	set_callback(ev, ev, mfp, 1);
    }

    // low priority assign:
    // - member function pointed by mfp is assigned to range of [ev_beg, ev_end].
    // - ev_end is included.
    // - this assignment can be overridden by higher priority assginment.
    void dispatcher_set(int32_t ev_beg, int32_t ev_end, memfunc_ptr mfp) {
	set_callback(ev_beg, ev_end, mfp, 0);
    }

private:
    struct {
	std::unordered_map<int32_t, std::pair<int, int>> event_map;
	std::vector<memfunc_ptr> callback_vec;
    } dispatcher_;

    void set_callback(int32_t ev_beg, int32_t ev_end, memfunc_ptr mfp, int prio) {
	auto& cbvec = dispatcher_.callback_vec;
	auto& evmap = dispatcher_.event_map;

	int vec_index = -1;
	if (auto it = find(cbvec.begin(), cbvec.end(), mfp); it != cbvec.end()) {
	    vec_index = it - cbvec.begin();
	}

	for (int ev = ev_beg; ev <= ev_end; ev++) {
	    if (auto it = evmap.find(ev); it == evmap.end()) {
		if (vec_index == -1) {
		    vec_index = cbvec.size();
		    cbvec.push_back(mfp);
		}
		evmap.emplace(ev, std::make_pair(prio, vec_index));
	    } else {
		auto [cur_prio, cur_vec] = (*it).second;
		if (cur_prio < prio) {
		    if (vec_index == -1) {
			vec_index = cbvec.size();
			cbvec.push_back(mfp);
		    }
		    (*it).second.first = prio;
		    (*it).second.second = vec_index;
		}
	    }
	}
    };
};


} // c7::event::ext


#endif // c7event/ext/dispatcher.hpp
