/*
 * c7event/service.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_SERVICE_HPP_LOADED__
#define C7_EVENT_SERVICE_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/monitor.hpp>
#include <c7event/port.hpp>


namespace c7::event {


// extract event number from Msgbuf
// ------------------------
// This is only primary template declaration without definition.
template <typename Msgbuf>
int32_t get_event(const Msgbuf& msgbuf);


typedef std::variant<void*, uint64_t> provider_hint;


class attach_id {
private:
    template <typename, typename>
    friend class service_interface;
    attach_id() = default;
};


class detach_id {
private:
    template <typename, typename>
    friend class service_interface;
    detach_id() = default;
};


template <typename Msgbuf, typename Port = socket_port>
class service_interface:
    public std::enable_shared_from_this<service_interface<Msgbuf, Port>> {
public:
    using attach_id = c7::event::attach_id;
    using detach_id = c7::event::detach_id;
    using base_type = service_interface<Msgbuf, Port>;
    using msgbuf_type = Msgbuf;
    using port_type = Port;

    service_interface() = default;
    virtual ~service_interface() = default;


    // interface for Receiver
    // ----------------------

    // [IMPORTANT] 
    //
    // If your delived class from service_interface implement on_attached, it must call
    // on_attached of base class like as following example. This requirement is also same
    // to on_detached.
    //
    //  class YourService: public YourBaseService {
    //      ...
    //      attach_id on_attached(monitor& mon, Port& port, provider_hint hint) override
    //      {
    //          auto id = YourBaseService::on_attached(mon, port, hint);
    //          .... your code ....
    //          return id;
    //	    }
    //
    virtual attach_id on_attached(monitor&, port_type&, provider_hint) { return attach_id(); }
    virtual detach_id on_detached(monitor&, port_type&, provider_hint) { return detach_id(); }

    // case: Msgbuf::recv() return io_result::status::OK
    virtual void on_message(monitor&, port_type&, msgbuf_type&) = 0;

    // case: Msgbuf::recv() return io_result::status::CLOSED
    //       If Port object is alive when returned from on_disconnect,
    //       Receiver call Port::close.
    virtual void on_disconnected(monitor&, port_type&, io_result&) {}

    // case: Msgbuf::recv() return others
    //       If Port object is alive when returned from on_disconnect,
    //       Receiver call Port::close.
    virtual void on_error(monitor&, port_type&, io_result&) {}


    // interface for connector
    // ----------------------

    // on_pre_conenct is called previous to calling connect system call.
    //
    // [ADVISE] Here is the only chance of setting SO_RCVBUF option, because it's no effect
    //          to set SO_RCVBUF option after connection is established.
    //
    virtual void on_pre_connect(monitor&, port_type&) {}
};


template <typename Msgbuf, typename Port>
using shared_service_ptr = std::shared_ptr<service_interface<Msgbuf, Port>>;


} // namespace c7::event


#endif // c7event/service.hpp
