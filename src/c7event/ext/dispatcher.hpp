/*
 * c7event/dispatcher.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_DISPATCHER_HPP_LOADED__
#define C7_EVENT_DISPATCHER_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/monitor.hpp>


namespace c7::event {


// extract event number from Msgbuf
// --------------------------------

template <typename Msgbuf>
int32_t get_event(const Msgbuf& msgbuf);


// event dispatch extention
// ------------------------
//
//  dispatcher template require two typename parameters. First is target class which is
//  about to be defined by you, and second is service base class. It is service_interface
//  in many case, but you intend to use other extenstion, you shoud specify it as second.
//
//  [Basic]
//    using extended_base = c7::event::fsm_service<service_interface<iov_msfbuf>>;
//
//    class my_service: public dispatcher<my_service, extended_base> {
//    public:
//        ...
//        // You must define setup_dispatcher and you setup callbacsk by dispatcher_set().
//        void dispatcher_setup() {
//            dispatcher_set(EventNumber, &my_service::handle_event);
//            ...
//        }
//          
//        // You must define callback_default.
//        void callback_default(monitor& mon, port_type& port, msgbuf_type& msg) override;
//
//        // Event handler has same signature with on_message.
//        void handle_event(monitor& mon, port_type& port, msgbuf_type& msg);
//
//        ...
//
//    };
//
//  [Advanced]
//    You can omit setup codes in the dispatcher_setup by using setup_dispatcher.py if you
//    specify callbacks declaration as follow.
//
//    class my_service: ... {
//        ...
//
//        // [TYPE 1]
//        // callback function name is delived by event mnemonic with callback_ prefix.
//
//        void callback_EVENT_NAME(monitor& mon, port_type& port, msgbuf_type& msg);
//
//        // [TYPE 2]
//        // you can assign one callback function to two or more events by specifying 
//        // special comment //[[dispatcher:callback EVENT_RANGE ...] ahead of declaration.
//        // EVENT_RANGE is single event number or CLOSED interval represented by two event
//        // numbers with infix colon. 
//
//        //[dispatcher:callback EVENT_NAME_1:EVENT_NAME_2 ...]
//        void callback_FAVORITE_NAME(monitor& mon, port_type& port, msgbuf_type& msg);
//
//        ...
//    };
//
//    // The dispatcher_setup must be defined out of class declaration and it include
//    // two special comments //[dispatcher:setup begin] and //[dispatcher:setup end].
//
//    // The setup_dispatcher.py replace the text surrounded by them with setup codes 
//    // delived from class declaration.
//
//    // [Additional]  When multiple functions assign to one event number, one is selected
//                     by their priotity. [TYPE 1] is higher than [TYPE 2]. First assigned
//                     is higher than later assigned in same TYPE.
//
//    void my_service::dispatcher_setup()
//    {
//        //[dispatcher:setup begin]
//        //[dispatcher:setup end]
//    }

template <typename DelivedService, typename BaseService>
class dispatcher: public BaseService {
public:
    using port_type = typename BaseService::port_type;
    using msgbuf_type = typename BaseService::msgbuf_type;
    using monitor   = c7::event::monitor;
    using memfunc_ptr = void (DelivedService::*)(monitor&, port_type&, msgbuf_type&);

    dispatcher() {
	static_cast<DelivedService*>(this)->dispatcher_setup();
    }

    void on_message(monitor& mon, port_type& port, msgbuf_type& msg) final {
	auto& evmap = dispatcher_.event_map;
	if (auto it = evmap.find(get_event(msg)); it != evmap.end()) {
	    auto [_, vec_index] = (*it).second;
	    (static_cast<DelivedService*>(this)->*dispatcher_.callback_vec[vec_index])(mon, port, msg);
	} else {
	    callback_default(mon, port, msg);
	}
    }

    virtual void callback_default(monitor& mon, port_type& port, msgbuf_type& msg) = 0;

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


} // c7::event


#endif // c7event/dispatcher.hpp
