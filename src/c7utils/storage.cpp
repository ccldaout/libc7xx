/*
 * c7utils/storage.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


//#include <c7defer.hpp>
//#include <c7format.hpp>
#include <c7utils/storage.hpp>
//#include <algorithm>
#include <cstdlib>
#include <cstring>


namespace c7 {


/*----------------------------------------------------------------------------
                              simple raw storage
----------------------------------------------------------------------------*/

storage::~storage()
{
    std::free(addr_);
}

storage::storage(storage&& o)
{
    addr_ = o.addr_;
    size_ = o.size_;
    align_ = o.align_;
    o.addr_ = nullptr;
    o.size_ = 0;
}

storage& storage::operator=(storage&& o)
{
    if (this != &o) {
	std::free(addr_);
	addr_ = o.addr_;
	size_ = o.size_;
	align_ = o.align_;
	o.addr_ = nullptr;
	o.size_ = 0;
    }
    return *this;
}

storage& storage::copy_from(const storage& o)
{
    if (this != &o) {
	if (auto res = reserve(o.size_); !res) {
	    throw std::runtime_error(c7::format("%{}", res));
	}
	std::memcpy(addr_, o.addr_, o.size_);
    }
    return *this;
}

storage storage::copy_to()
{
    storage new_storage{align_};
    new_storage.copy_from(*this);
    return new_storage;
}

result<> storage::reserve(size_t req)
{
    auto new_size = c7_align(req, align_);
    if (size_ < new_size) {
	if (void *new_addr = std::realloc(addr_, new_size); new_addr == nullptr) {
	    return c7result_err(errno, "cannot allocate memory");
	} else {
	    addr_ = new_addr;
	    size_ = new_size;
	}
    }
    return c7result_ok();
}

void storage::reset()
{
    std::free(addr_);
    addr_ = nullptr;
    size_ = 0;
}

void storage::clear()
{
    std::memset(addr_, 0, size_);
}


} // namespace c7
