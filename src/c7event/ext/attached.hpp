/*
 * c7event/ext/attached.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_EXT_ATTACHED_HPP_LOADED_
#define C7_EVENT_EXT_ATTACHED_HPP_LOADED_
#include <c7common.hpp>


#include <c7event/portgroup.hpp>
#include <c7event/service.hpp>


namespace c7::event::ext {


template <typename BaseService>
class attached: public BaseService {
public:
    using port_type    = typename BaseService::port_type;
    using msgbuf_type  = typename BaseService::msgbuf_type;

    portgroup<port_type> ext_attached;

    attached() {}

    attach_id on_attached(monitor& mon, port_type& port, provider_hint hint) override {
	ext_attached.add(port);
	auto id = BaseService::on_attached(mon, port, hint);
	return id;
    }

    detach_id on_detached(monitor& mon, port_type& port, provider_hint hint) override {
	ext_attached.remove(port);
	auto id = BaseService::on_detached(mon, port, hint);
	return id;
    }

private:
};


} // c7::event::ext


#endif // c7event/ext/attached.hpp
