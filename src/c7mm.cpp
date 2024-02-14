/*
 * c7mm.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7mm.hpp>
#include <c7path.hpp>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <cstring>


#define PAGESIZE	sysconf(_SC_PAGESIZE)


namespace c7 {


/*----------------------------------------------------------------------------
                            anonymous mmap manager
----------------------------------------------------------------------------*/

anon_mmap_manager::anon_mmap_manager(anon_mmap_manager&& o):
    map_size_(o.map_size_), map_addr_(o.map_addr_)
{
    o.map_size_ = 0;
    o.map_addr_ = nullptr;
}

anon_mmap_manager&
anon_mmap_manager::operator=(anon_mmap_manager&& o)
{
    if (this != &o) {
	if (map_addr_ != nullptr) {
	    ::munmap(map_addr_, map_size_);
	}
	map_size_ = o.map_size_;
	map_addr_ = o.map_addr_;
	o.map_size_ = 0;
	o.map_addr_ = nullptr;
    }
    return *this;
}

c7::result<std::pair<void *, size_t>>
anon_mmap_manager::reserve(size_t size)
{
    size = c7_align(size, PAGESIZE);
    if (size != map_size_) {
	void *addr;
	if (map_size_ == 0) {
	    addr = ::mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	} else {
	    addr = ::mremap(map_addr_, map_size_, size, MREMAP_MAYMOVE);
	}
	if (addr == MAP_FAILED) {
	    return c7result_err(errno, "mmap/mremap(size:%{}) failed", size);
	}
	map_addr_ = addr;
	map_size_ = size;
    }
    return c7result_ok(std::make_pair(map_addr_, map_size_));
}

void
anon_mmap_manager::reset()
{
    if (map_addr_ != nullptr) {
	::munmap(map_addr_, map_size_);
	map_addr_ = nullptr;
	map_size_ = 0;
    }
}


/*----------------------------------------------------------------------------
                            switchable mmap object
----------------------------------------------------------------------------*/

result<> mmobj::resize_anon_mm(size_t new_size)
{
    new_size = c7_align(new_size, PAGESIZE);
    if (new_size == 0 || size_ == new_size) {
	return c7result_ok();
    }
    void *new_top;
    if (size_ == 0) {
	new_top = ::mmap(nullptr, new_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    } else {
	new_top = ::mremap(top_, size_, new_size, MREMAP_MAYMOVE);
    }

    if (new_top == MAP_FAILED) {
	if (path_.empty()) {
	    return c7result_err(errno, "mmap/mremap(size:%{}) failed", new_size);
	}
	return switch_to_file_mm(new_size);
    }
    top_  = new_top;
    size_ = new_size;
    return c7result_ok();
}

result<> mmobj::switch_to_file_mm(size_t new_size)
{
    new_size = c7_align(new_size, PAGESIZE);

    result<c7::fd> res1;
    if (c7::path::is_dir(path_)) {
	res1 = c7::opentmp(path_);
    } else {
	res1 = c7::open(path_, O_RDWR|O_CREAT);
    }
    if (!res1) {
	return res1.as_error();
    }
    fd_ = std::move(res1.value());

    if (auto res = fd_.truncate(new_size); !res) {
	return res;
    }

    void *new_top = ::mmap(nullptr, new_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd_, 0);
    if (new_top == MAP_FAILED) {
	return c7result_err(errno, "mmap(nullptr, %{}, ..., %{}, 0) failed", new_size, fd_);
    }
    if (top_ != nullptr) {
	(void)std::memcpy(new_top, top_, size_);
	(void)::munmap(top_, size_);
    }
    top_  = new_top;
    size_ = new_size;
    return c7result_ok();
}

result<> mmobj::resize_file_mm(size_t new_size)
{
    new_size = c7_align(new_size, PAGESIZE);
    if (auto res = fd_.truncate(new_size); !res) {
	return res;
    }
    void *new_top = ::mmap(top_, new_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd_, 0);
    if (new_top == MAP_FAILED) {
	return c7result_err(errno, "mmap(%{}, %{}, ..., %{}, 0) failed", top_, new_size, fd_);
    }
    if (new_top != top_) {
	// std::memcpy is not needed in MAP_SHARED file mapping
	(void)::munmap(top_, size_);
	top_  = new_top;
    }
    size_ = new_size;
    return c7result_ok();
}

mmobj::~mmobj()
{
    if (top_ != nullptr) {
	(void)::munmap(top_, size_);
    }
}

mmobj::mmobj(mmobj&& o)
{
    top_  = o.top_;
    size_ = o.size_;
    threshold_ = o.threshold_;
    path_      = std::move(o.path_);
    fd_        = std::move(o.fd_);
    o.top_  = nullptr;
    o.size_ = 0;
}

mmobj& mmobj::operator=(mmobj&& o)
{
    if (this != &o) {
	if (top_ != nullptr) {
	    ::munmap(top_, size_);
	}
	top_  = o.top_;
	size_ = o.size_;
	path_ = std::move(o.path_);
	fd_   = std::move(o.fd_);
	o.top_  = nullptr;
	o.size_ = 0;
    }
    return *this;
}

result<> mmobj::init(size_t size, size_t threshold, const std::string& path)
{
    reset();
    threshold_ = threshold;
    path_ = path;
    return resize(size);
}

result<> mmobj::resize(size_t new_size)
{
    if (fd_) {
	return resize_file_mm(new_size);
    }
    if (new_size > threshold_) {
	return switch_to_file_mm(new_size);
    }
    return resize_anon_mm(new_size);
}

void mmobj::reset()
{
    *this = mmobj();
}

std::pair<void*, size_t> mmobj::operator()()
{
    return { top_, size_ };
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7
