/*
 * c7event/portgroup.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */


#include "portgroup.hpp"


namespace c7::event {


portgroup<shared_port>::~portgroup()
{
    for (auto& [port, id]: ports_) {
	port.remove_on_close(id);
    }
}


void
portgroup<shared_port>::add(shared_port& port)
{
    auto unlock = lock();
    auto id = port.add_on_close(
	[this, fd=port.fd_number()]() { remove(fd); });
    ports_.emplace_back(port, id);
}


void
portgroup<shared_port>::remove(shared_port& port)
{
    remove(port.fd_number());
}


void
portgroup<shared_port>::remove(int fd)
{
    auto unlock = lock();
    auto it = std::find_if(ports_.begin(), ports_.end(),
			   [fd](auto p){
			       return (p.first.fd_number() == fd);
			   });
    if (it != ports_.end()) {
	auto& [port, id] = *it;
	port.remove_on_close(id);
	ports_.erase(it);
    }
}


std::vector<std::pair<shared_port*, io_result>>
portgroup<shared_port>::errors()
{
    std::vector<std::pair<shared_port*, io_result>> errs;
    for (auto& [p, iores]: errs_) {
	io_result new_iores;
	new_iores.copy_from(iores);
	errs.emplace_back(&p, std::move(new_iores));
    }
    return errs;
}


std::vector<std::pair<const shared_port*, io_result>>
portgroup<shared_port>::errors() const
{
    std::vector<std::pair<const shared_port*, io_result>> errs;
    for (auto& [p, iores]: errs_) {
	io_result new_iores;
	new_iores.copy_from(iores);
	errs.emplace_back(&p, std::move(new_iores));
    }
    return errs;
}


} // namespace c7::event
