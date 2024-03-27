/*
 * c7event/ext/noop.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_EXT_NOOP_HPP_LOADED_
#define C7_EVENT_EXT_NOOP_HPP_LOADED_
#include <c7common.hpp>


#include <c7event/service.hpp>


namespace c7::event::ext {


template <typename BaseService>
class noop_service: public BaseService {
public:
    using port_type = typename BaseService::port_type;
    using msgbuf_type = typename BaseService::msgbuf_type;

    noop_service() {}
    void on_message(monitor&, port_type&, msgbuf_type&) override {}
};


} // c7::event::ext


#endif // c7event/ext/noop.hpp
