/*
 * c7event/port.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7event/port.hpp>


namespace c7::event {


socket_port socket_port::tcp()
{
    if (auto res = c7::socket::tcp(); res) {
	return socket_port(std::move(res.value()));
    }
    return socket_port(c7::socket());
}

socket_port socket_port::unix()
{
    if (auto res = c7::socket::unix(); res) {
	return socket_port(std::move(res.value()));
    }
    return socket_port(c7::socket());
}


} // namespace c7::event
