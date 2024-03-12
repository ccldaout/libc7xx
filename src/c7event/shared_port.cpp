/*
 * c7event/shared_port.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7event/shared_port.hpp>


namespace c7::event {


shared_port::impl::impl(c7::socket&& sock):
    port(std::move(sock))
{
}


shared_port::impl::impl(int fd):
    port(fd)
{
}


shared_port::impl::impl(socket_port&& port):
    port(std::move(port))
{
}


shared_port::shared_port(std::shared_ptr<impl>&& pimpl):
    pimpl_(std::move(pimpl))
{
}


shared_port::shared_port(c7::socket&& sock):
    pimpl_(new impl(std::move(sock))) 
{
}


shared_port::shared_port(int fd):
    pimpl_(new impl(fd))
{
}


shared_port::shared_port(socket_port&& port):
    pimpl_(new impl(std::move(port)))
{
}


shared_port
shared_port::tcp()
{
    if (auto res = c7::socket::tcp(); res) {
	return shared_port(std::move(res.value()));
    }
    return shared_port(c7::socket());
}


shared_port
shared_port::unix()
{
    if (auto res = c7::socket::unix(); res) {
	return shared_port(std::move(res.value()));
    }
    return shared_port(c7::socket());
}


result<shared_port>
shared_port::accept()
{
    if (auto res = pimpl_->port.accept(); !res) {
	return c7result_err(std::move(res));
    } else {
	return c7result_ok(shared_port(std::move(res.value())));
    }
}


void
shared_port::print(std::ostream& out, const std::string&) const
{
    c7::format(out, "shared<%{}>", *(pimpl_->port.operator->()));
}


} // namespace c7::event
