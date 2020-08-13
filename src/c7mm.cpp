/*
 * c7mm.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include "c7mm.hpp"
#include "c7path.hpp"
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <cstring>


namespace c7 {


/*----------------------------------------------------------------------------
                            switchable mmap object
----------------------------------------------------------------------------*/

result<void> mmobj::init_anon_mm(size_t size)
{
    return resize_anon_mm(size);
}

result<void> mmobj::resize_anon_mm(size_t size)
{
    size = c7_align(size, PAGESIZE);
    if (size == 0) {
	return c7result_ok();
    }	
    void *np = ::mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (np == MAP_FAILED) {
	if (path_.empty()) {
	    return c7result_err(errno, "mmap(nullptr, %{}, ..., -1, 0) failed", size);
	}
	return switch_to_file_mm(size);
    }
    if (top_ != nullptr) {
	(void)std::memcpy(np, top_, size_);
	(void)::munmap(top_, size_);
    }
    top_  = np;
    size_ = size;
    return c7result_ok();
}

result<void> mmobj::switch_to_file_mm(size_t size)
{
    size = c7_align(size, PAGESIZE);

    result<c7::fd> res1;
    if (c7::path::is_dir(path_)) {
	res1 = c7::opentmp(path_);
    } else {
	res1 = c7::open(path_, O_RDWR|O_CREAT);
    }
    if (!res1) {
	return res1;
    }
    fd_ = std::move(res1.value());

    auto res2 = fd_.stat();
    if (!res2) {
	return res2;
    }
    auto fsize = res2.value().st_size;

    if (size > (size_t)fsize) {
	if (auto res = fd_.truncate(size); !res)
	    return res;
    } else {
	size = fsize;
    }

    void *np = ::mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd_, 0);
    if (np == MAP_FAILED) {
	return c7result_err(errno, "mmap(nullptr, %{}, ..., %{}, 0) failed", size, fd_);
    }
    if (top_ != nullptr) {
	(void)std::memcpy(np, top_, size_);
	(void)::munmap(top_, size_);
    }
    top_  = np;
    size_ = size;
    return c7result_ok();
}

result<void> mmobj::resize_file_mm(size_t size)
{
    size = c7_align(size, PAGESIZE);
    if (auto res = fd_.truncate(size); !res)
	return res;
    void *np = ::mmap(top_, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd_, 0);
    if (np == MAP_FAILED) {
	return c7result_err(errno, "mmap(%{}, %{}, ..., %{}, 0) failed", top_, size, fd_);
    }
    if (np != top_) {
	// std::memcpy is not needed in MAP_SHARED file mapping
	(void)::munmap(top_, size_);
	top_  = np;
    }
    size_ = size;
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
    threshold_ = threshold_;
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

result<void> mmobj::init(size_t size, size_t threshold, const std::string& path)
{
    threshold_ = threshold;
    path_ = path;
    return resize(size);
}
    
result<void> mmobj::resize(size_t new_size)
{
    if (fd_) {
	return resize_file_mm(new_size);
    }
    if (new_size > threshold_) {
	return switch_to_file_mm(new_size);
    }
    return resize_anon_mm(new_size);
}

std::pair<void*, size_t> mmobj::operator()()
{
    return { top_, size_ };
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7
