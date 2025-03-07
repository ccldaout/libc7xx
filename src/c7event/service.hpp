/*
 * c7event/service.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_SERVICE_HPP_LOADED_
#define C7_EVENT_SERVICE_HPP_LOADED_
#include <c7common.hpp>


#include <c7event/monitor.hpp>
#include <c7event/port.hpp>


namespace c7::event {


// extract event number from Msgbuf
// ------------------------
// This is only primary template declaration without definition.
template <typename Msgbuf>
int32_t get_event(const Msgbuf& msgbuf);


class provider_hint: public std::variant<void*, uint64_t> {
private:
    using base_type = std::variant<void*, uint64_t>;

public:
    constexpr provider_hint() = default;
    constexpr provider_hint(const provider_hint&) = default;
    constexpr provider_hint(provider_hint&&) = default;
    constexpr provider_hint& operator=(const provider_hint&) = default;
    constexpr provider_hint& operator=(provider_hint&&) = default;

    template <typename T,
	      typename = std::enable_if_t<!std::is_pointer_v<c7::typefunc::remove_cref_t<T>>>>
    constexpr provider_hint(T v): base_type(static_cast<uint64_t>(v)) {}

    constexpr provider_hint(std::nullptr_t): base_type(nullptr) {}

    constexpr provider_hint(const void *p): base_type(const_cast<void *>(p)) {}

    template <typename T,
	      typename = std::enable_if_t<std::disjunction_v<std::is_integral<T>,
							     std::is_enum<T>>>>
    constexpr bool operator==(const T& v) const {
	return (std::get<1>(static_cast<const base_type&>(*this)) == static_cast<uint64_t>(v));
    }

    constexpr bool operator==(std::nullptr_t) const {
	return (std::get<0>(static_cast<const base_type&>(*this)) == nullptr);
    }

    template <typename T>
    constexpr bool operator==(const T *p) const {
	return (std::get<0>(static_cast<const base_type&>(*this)) == static_cast<const void*>(p));
    }

    constexpr bool operator==(const provider_hint& o) const {
	return (static_cast<const base_type&>(*this) == static_cast<const base_type&>(o));
    }

    template <typename T,
	      typename = std::enable_if_t<std::disjunction_v<std::is_integral<T>,
							     std::is_enum<T>>>>
    constexpr bool operator!=(const T& v) const {
	return !(*this == v);
    }

    constexpr bool operator!=(std::nullptr_t) const {
	return !(*this == nullptr);
    }

    template <typename T>
    constexpr bool operator!=(const T *p) const {
	return !(*this == p);
    }

    constexpr bool operator!=(const provider_hint& o) const {
	return !(*this == o);
    }
};


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
    using provider_hint = c7::event::provider_hint;
    using monitor   = c7::event::monitor;
    using attach_id = c7::event::attach_id;
    using detach_id = c7::event::detach_id;
    using msgbuf_type = Msgbuf;
    using port_type = Port;

    service_interface() = default;
    virtual ~service_interface() = default;


    // interface for receiver
    // ----------------------

    // [IMPORTANT]
    //
    // If your derived class from service_interface implement on_attached, it must call
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
    virtual void on_message(monitor&, port_type&, msgbuf_type&) {}

    // case: Msgbuf::recv() return io_result::status::CLOSED
    //       If Port object is alive when returned from on_disconnect,
    //       receiver call Port::close.
    virtual void on_disconnected(monitor&, port_type&, io_result&) {}

    // case: Msgbuf::recv() return others
    //       If Port object is alive when returned from on_disconnect,
    //       receiver call Port::close.
    virtual void on_error(monitor&, port_type&, io_result&) {}


    // interface for connector
    // ----------------------

    // on_pre_conenct is called previous to calling connect system call.
    //
    // [ADVISE] Here is the only chance of setting SO_RCVBUF option, because it's no effect
    //          to set SO_RCVBUF option after connection is established.
    //
    virtual void on_pre_connect(monitor&, port_type&) {}

private:
    using base_type = service_interface<Msgbuf, Port>;
};


template <typename Msgbuf, typename Port>
using shared_service_ptr = std::shared_ptr<service_interface<Msgbuf, Port>>;


} // namespace c7::event


#endif // c7event/service.hpp
