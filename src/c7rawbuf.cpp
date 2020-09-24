/*
 * c7rawbuf.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7rawbuf.hpp>


namespace c7 {


std_memory_manager::std_memory_manager(std_memory_manager&& o):
    size_(o.size_), addr_(o.addr_)
{
    o.size_ = 0;
    o.addr_ = nullptr;
}

std_memory_manager&
std_memory_manager::operator=(std_memory_manager&& o)
{
    if (this != &o) {
	if (addr_ != nullptr) {
	    std::free(addr_);
	}
	size_ = o.size_;
	addr_ = o.addr_;
	o.size_ = 0;
	o.addr_ = nullptr;
    }
    return *this;
}

c7::result<std::pair<void *, size_t>>
std_memory_manager::reserve(size_t size)
{
    if (size <= size_) {
	return c7result_ok();
    }
    if (void *addr = std::realloc(addr_, size); addr == nullptr) {
	return c7result_err(errno, "std::realloc(, %{}) failed", size);
    } else {
	addr_ = addr;
	size_ = size;
	return c7result_ok(std::make_pair(addr_, size_));
    }
}

void
std_memory_manager::reset()
{
    std::free(addr_);
    addr_ = nullptr;
    size_ = 0;
}


} // namespace c7
