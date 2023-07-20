/*
 * c7file.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1718128885
 */
#ifndef C7_FILE_HPP_LOADED__
#define C7_FILE_HPP_LOADED__
#include <c7common.hpp>


#include <c7result.hpp>
#include <c7utils.hpp>
#include <fcntl.h>
#include <sys/mman.h>
#include <memory>
#include <string>


namespace c7 {
namespace file {


// recursive mkdir

c7::result<> mkdir(const std::string& path,
		       mode_t mode = 0700, uid_t uid = -1, gid_t gid = -1);

// set same owner of parent directory

c7::result<> inherit_owner(const std::string& path);


// file read/write operations

c7::result<> write(const std::string& path, mode_t mode, const void *buf, size_t size);

template <typename T>
c7::result<> write(const std::string& path, mode_t mode, const T& buf)
{
    return write(path, mode, &buf, sizeof(buf));
}

c7::result<> rewrite(const std::string& path, void *buf, size_t size);

template <typename T>
c7::result<> rewrite(const std::string& path, const T& buf)
{
    return rewrite(path, &buf, sizeof(buf));
}

c7::result<ssize_t> read_into(const std::string& path, void *buf, size_t size);

template <typename T>
c7::result<ssize_t> read_into(const std::string& path, T& buf)
{
    return read_into(path, &buf, sizeof(buf));
}

result<void*> read_impl(const std::string& path, size_t& size);

template <typename T = void>
c7::result<c7::unique_cptr<T>> read(const std::string& path, size_t& size_io)
{
    auto res = read_impl(path, size_io);
    if (!res)
	return c7result_err(std::move(res), "c7::file::read failed");

    return c7result_ok(c7::make_unique_cptr<T>(static_cast<T*>(res.value())));
}

template <typename T = void>
c7::result<c7::unique_cptr<T>> read(const std::string& path)
{
    size_t size = 0;
    return read<T>(path, size);
}

result<std::vector<std::string>> readlines(const std::string& path);


// convenience mmap operations

class mmap_deleter {
public:
    mmap_deleter() = default;
    explicit mmap_deleter(size_t n): n_(n) {}
    void operator()(void *p) {
	::munmap(p, n_);
    }
private:
    size_t n_ = 0;
};

template <typename T = void>
using unique_mmap = std::unique_ptr<T, mmap_deleter>;

result<void*> mmap_impl(const std::string& path, size_t& size_io, int oflag);
result<void*> mmap_impl(int fd, size_t& size_io, int oflag);

template <typename T = void>
c7::result<unique_mmap<T>> mmap_r(const std::string& path, size_t& size_io)
{
    auto res = mmap_impl(path, size_io, O_RDONLY);
    if (!res) {
	return c7result_err(std::move(res), "c7::file::mmap_r failed");
    }
    return c7result_ok(unique_mmap<T>(static_cast<T*>(res.value()),
				      mmap_deleter(size_io)));
}

template <typename T = void>
c7::result<unique_mmap<T>> mmap_r(int fd, size_t& size_io)
{
    auto res = mmap_impl(fd, size_io, O_RDONLY);
    if (!res) {
	return c7result_err(std::move(res), "c7::file::mmap_r failed");
    }
    return c7result_ok(unique_mmap<T>(static_cast<T*>(res.value()),
				      mmap_deleter(size_io)));
}

template <typename T = void>
inline c7::result<unique_mmap<T>> mmap_r(const std::string& path)
{
    size_t size = 0;
    // Template paramter <T> of next calling is very important !!.
    // if it is not specified, result<R>(result_base&&) constructor is called on
    // next return statement, and after that unique_ptr destructor is called.
    return mmap_r<T>(path, size);
}

template <typename T = void>
inline c7::result<unique_mmap<T>> mmap_r(int fd)
{
    size_t size = 0;
    return mmap_r<T>(fd, size);
}

template <typename T = void>
c7::result<unique_mmap<T>> mmap_rw(const std::string& path, size_t& size_io, bool create)
{
    int oflag = O_RDWR;
    if (create)
	oflag |= O_CREAT;

    auto res = mmap_impl(path, size_io, oflag);
    if (!res) {
	return c7result_err(std::move(res), "c7::file::mmap_rw failed");
    }
    return c7result_ok(unique_mmap<T>(static_cast<T*>(res.value()),
				      mmap_deleter(size_io)));
}

template <typename T = void>
c7::result<unique_mmap<T>> mmap_rw(int fd, size_t& size_io, bool create)
{
    int oflag = O_RDWR;
    if (create) {
	oflag |= O_CREAT;
    }
    auto res = mmap_impl(fd, size_io, oflag);
    if (!res) {
	return c7result_err(std::move(res), "c7::file::mmap_rw failed");
    }
    return c7result_ok(unique_mmap<T>(static_cast<T*>(res.value()),
				      mmap_deleter(size_io)));
}

template <typename T = void>
c7::result<unique_mmap<T>> mmap_rw(const std::string& path)
{
    size_t size = 0;
    return mmap_rw<T>(path, size, false);
}

template <typename T = void>
c7::result<unique_mmap<T>> mmap_rw(int fd)
{
    size_t size = 0;
    return mmap_rw<T>(fd, size, false);
}


} // namespace file
} // namespace c7


#endif // c7file.hpp
