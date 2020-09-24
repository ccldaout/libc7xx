/*
 * c7fd.hpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_FD_HPP_LOADED__
#define C7_FD_HPP_LOADED__
#include <c7common.hpp>


#include <c7delegate.hpp>
#include <c7result.hpp>
#include <sys/stat.h>
#include <sys/uio.h>


namespace c7 {


/*----------------------------------------------------------------------------
                                  I/O result
----------------------------------------------------------------------------*/

class io_result {
public:
    enum class status {
	OK,		// *     : all data has been processed successfully
	BUSY,		// *     : detect EWOULDBLOCK before read first byte.
	CLOSED,		// read_n: detect CLOSED before read first byte.
	INCOMP,		// read_n: detect CLOSED in reading more data.
	ERR,		// *     : detect other ERROR
    };

private:
    c7::result_base result_;
    status status_ = status::OK;
    size_t done_   = 0;
    size_t remain_ = 0;

public:
    io_result(const io_result&) = delete;
    io_result& operator=(const io_result&) = delete;

    io_result(io_result&& o);
    io_result& operator=(io_result&& o);

    io_result();
    io_result(status status, size_t done_, size_t remain, c7::result_base&& base);

    operator bool() const {
	return (status_ == status::OK);
    }

    c7::result_base& get_result() {
	return result_;
    }

    status get_status() const {
	return status_;
    }

    size_t get_remain() const {
	return remain_;
    }

    static std::string format_status(status status);
    std::string format(const std::string& spec) const;
};


/*----------------------------------------------------------------------------
                                      fd
----------------------------------------------------------------------------*/

class fd {
protected:
    int fdnum_ = -1;
    std::string name_;
    mutable std::string str_;

    void _close();

public:
    enum available {
	READABLE  = (1U << 0),
	WRITABLE = (1U << 1),
    };

    typedef struct ::stat stat_t;

    c7::delegate<void, fd&> on_close;

    fd(const fd&) = delete;
    fd& operator=(const fd&) = delete;

    virtual ~fd() {
	_close();
    }

    fd();
    explicit fd(int fdnum, const std::string& name = "");
    fd(fd&& o);
    fd& operator=(fd&& o);

    operator int() const {
	return fdnum_;
    }

    operator bool() const {
	return fdnum_ != -1;
    }

    fd& swap(fd& o);
    result<fd> dup(); 
    result<void> renumber_to(int target_fdnum);
    result<void> renumber_above(int lowest_fdnum);

    result<int> get_flag(int flagtype);
    result<void> set_flag(int flagtype, int flag);
    result<void> change_flag(int set_type, int flags, bool enable);

    result<bool> get_cloexec();
    result<void> set_cloexec(bool enable);

    result<bool> get_nonblocking();
    result<void> set_nonblocking(bool enable);

    result<void> chmod(::mode_t mode);
    result<void> chown(uid_t uid);
    result<void> chown(uid_t uid, gid_t gid);
    result<void> truncate(size_t size);
    result<stat_t> stat() const;

    result<off_t> seek_abs(off_t offset);
    result<off_t> seek_cur(off_t offset);

    result<size_t> read(void *bufaddr, size_t size);
    result<size_t> write(const void *bufaddr, size_t size);

    io_result read_n(void *bufaddr, size_t req_n);
    io_result write_n(const void *bufaddr, size_t req_n);
    io_result write_v(::iovec*& iov_io, int& ioc_io);

    result<uint32_t> wait(uint32_t which, c7::usec_t tmo_us);

    result<uint32_t> wait(c7::usec_t tmo_us = -1) {
	return wait(READABLE|WRITABLE, tmo_us);
    }

    result<bool> wait_readable(c7::usec_t tmo_us = -1) {
	auto res = wait(READABLE, tmo_us);
	if (!res)
	    return res;
	return c7result_ok((res.value() & READABLE) != 0);
    }

    result<bool> wait_writable(c7::usec_t tmo_us = -1) {
	auto res = wait(WRITABLE, tmo_us);
	if (!res)
	    return res;
	return c7result_ok((res.value() & WRITABLE) != 0);
    }

    virtual void close() {
	_close();
    }

    const std::string& name() const {
	return name_;
    }

    void set_name(const std::string& name) {
	name_ = name;
	str_.clear();
    }

    void set_name(std::string&& name) {
	name_ = std::move(name);
	str_.clear();
    }

    virtual const std::string& format(const std::string spec) const;
};

result<fd> open(std::string&&, int oflag = 0, ::mode_t mode = 0600);
result<fd> open(const std::string&, int oflag = 0, ::mode_t mode = 0600);
result<fd> opentmp(const std::string& dir, ::mode_t mode = 0600);


/*----------------------------------------------------------------------------
                                     pipe
----------------------------------------------------------------------------*/

class pipe: public fd {
protected:
    static std::atomic<size_t> id_counter_;
    size_t id_;

public:
    pipe(const pipe&) = delete;
    pipe& operator=(const pipe&) = delete;

    pipe(): fd(), id_(++id_counter_) {}

    explicit pipe(int fdnum, const std::string& name = ""):
	fd(fdnum, name), id_(++id_counter_) {
    }

    pipe(pipe&& o):
	fd(std::move(o)), id_(++id_counter_) {
    }

    pipe& operator=(pipe&& o) {
	fd::operator=(std::move(o));
	id_ = o.id_;
	return *this;
    }

    virtual const std::string& format(const std::string spec) const;
};

result<std::pair<pipe, pipe>> make_pipe();


/*----------------------------------------------------------------------------
                                format traits
----------------------------------------------------------------------------*/

template <>
struct format_traits<io_result::status> {
    inline static void convert(std::ostream& out, const std::string& format, io_result::status arg) {
	out << io_result::format_status(arg);
    }
};

template <>
struct format_traits<io_result> {
    inline static void convert(std::ostream& out, const std::string& format, const io_result& arg) {
	out << arg.format(format);
    }
};

template <>
struct format_traits<fd> {
    inline static void convert(std::ostream& out, const std::string& format, const fd& arg) {
	out << arg.format(format);
    }
};

template <>
struct format_traits<pipe> {
    inline static void convert(std::ostream& out, const std::string& format, const pipe& arg) {
	out << arg.format(format);
    }
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7fd.hpp
