/*
 * c7event/noop.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_NOOP_HPP_LOADED__
#define C7_EVENT_NOOP_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/service.hpp>


namespace c7::event {


template <typename BaseService>
class noop_service: public BaseService {
public:
    using port_type = typename BaseService::port_type;
    using msgbuf_type = typename BaseService::msgbuf_type;
    using monitor   = c7::event::monitor;

    noop_service() {}
    void on_message(monitor&, port_type&, msgbuf_type&) override {}
};


} // c7::event


#endif // c7event/ext/noop.hpp
