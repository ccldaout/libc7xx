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


void
shared_port::print(std::ostream& out, const std::string&) const
{
    if (ops_) {
	c7::format(out, "shared<%{}>", ops_->socket());
    } else {
	out << "shared<nullptr>";
    }
}


void
weak_port::print(std::ostream& out, const std::string&) const
{
    auto spimpl = w_ops_.lock();
    if (spimpl) {
	c7::format(out, "weak<%{}>", spimpl->socket());
    } else {
	out << "weak<nullptr>";
    }
}


} // namespace c7::event
