/*
 * c7fd.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1541005293
 */
#ifndef C7_FD_HPP_LOADED_
#define C7_FD_HPP_LOADED_
#include <c7common.hpp>


#include <c7delegate.hpp>
#include <c7result.hpp>
#include <sys/stat.h>
#include <sys/uio.h>


#define C7_IO_RESULT_AS_ERROR	(1U)


namespace c7 {


/*----------------------------------------------------------------------------
                                  I/O result
----------------------------------------------------------------------------*/

class io_result {
public:
    enum class status {
	OK,		// *     : all data has been processed successfully
	BUSY,		// *     : detect EWOULDBLOCK
	CLOSED,		// read_n: detect CLOSED before read first byte.
	INCOMP,		// read_n: detect CLOSED in reading more data.
	ERR,		// *     : detect other ERROR
    };

    io_result(const io_result&) = delete;
    io_result& operator=(const io_result&) = delete;

    static io_result ok(size_t done = 0) {
	return io_result(status::OK, done, 0, c7result_ok());
    }

    template <typename Who>
    static io_result closed(const Who& who, size_t remain = 0) {
	return io_result(status::CLOSED, 0, remain,
			 c7result_err(ENODATA, "%{}: maybe closed", who));
    }

    template <typename Who>
    static io_result incomp(const Who& who, size_t done, size_t remain) {
	return io_result(status::INCOMP, done, remain,
			 c7result_err(ENODATA, "%{}: done:%{}, remain:%{}, maybe closed",
				      who, done, remain));
    }

    template <typename Who>
    static io_result busy(const Who& who) {
	return io_result(status::BUSY, 0, 0,
			 c7result_err(EWOULDBLOCK, "%{}: busy", who));
    }

    template <typename Who>
    static io_result error(const Who& who) {
	return io_result(status::ERR, 0, 0,
			 c7result_err(errno, "%{}: error", who));
    }

    template <typename Who>
    static io_result error(const Who& who, c7::result_base&& result) {
	return io_result(status::ERR, 0, 0, std::move(result));
    }

    io_result() {}
    io_result(status status, size_t done, size_t remain, c7::result_base&& result):
	result_(std::move(result)), status_(status), done_(done), remain_(remain) {}
    io_result(io_result&& o):
	result_(std::move(o.result_)), status_(o.status_), done_(o.done_), remain_(o.remain_) {}

    io_result& operator=(io_result&& o) {
	if (this != &o) {
	    result_ = std::move(o.result_);
	    status_ = o.status_;
	    done_   = o.done_;
	    remain_ = o.remain_;
	}
	return *this;
    }

    io_result& copy_from(const io_result& o) {
	if (this != &o) {
	    result_.copy_from(o.result_);
	    status_ = o.status_;
	    done_   = o.done_;
	    remain_ = o.remain_;
	}
	return *this;
    }

    explicit operator bool() const {
	return (status_ == status::OK);
    }

    operator const c7::result_base&() const {
	return result_;
    }

    operator c7::result_base&() {
	return result_;
    }

    c7::result_base& get_result() {
	return result_;
    }

    const c7::result_base& get_result() const {
	return result_;
    }

    c7::result_err&& as_error() {
	return result_.as_error();
    }

    const c7::result_err& as_error() const {
	return result_.as_error();
    }

    status get_status() const {
	return status_;
    }

    size_t get_remain() const {
	return remain_;
    }

    size_t get_done() const {
	return done_;
    }

    static std::string to_string(status status);
    std::string to_string(const std::string& spec) const;
    void print(std::ostream& out, const std::string& spec) const;

private:
    c7::result_base result_;
    status status_ = status::OK;
    size_t done_   = 0;
    size_t remain_ = 0;
};

void print_type(std::ostream& out, const std::string& format, io_result::status arg);


/*----------------------------------------------------------------------------
                                      fd
----------------------------------------------------------------------------*/

class fd {
public:
    enum available {
	READABLE  = (1U << 0),
	WRITABLE = (1U << 1),
    };

    using stat_t = struct stat;

    c7::delegate<void, fd&> on_close;

    fd(const fd&) = delete;
    fd& operator=(const fd&) = delete;

    virtual ~fd() {
	close();
    }

    fd();
    explicit fd(int fdnum, const std::string& name = "");
    fd(fd&& o);
    fd& operator=(fd&& o);

    result<> close();

    operator int() const {
	return fdnum_;
    }

    explicit operator bool() const {
	return fdnum_ != -1;
    }

    result<> renumber_to(int target_fdnum);
    result<> renumber_above(int lowest_fdnum);

    result<int> get_flag(int flagtype);
    result<> set_flag(int flagtype, int flag);
    result<> change_flag(int set_type, int flags, bool enable);

    result<bool> get_cloexec();
    result<> set_cloexec(bool enable);

    result<bool> get_nonblocking();
    result<> set_nonblocking(bool enable);

    result<> chmod(::mode_t mode);
    result<> chown(uid_t uid);
    result<> chown(uid_t uid, gid_t gid);
    result<> truncate(size_t size);
    result<stat_t> stat() const;

    result<off_t> seek_abs(off_t offset);
    result<off_t> seek_cur(off_t offset);

    result<size_t> read(void *bufaddr, size_t size);
    result<size_t> write(const void *bufaddr, size_t size);
    template <typename T> result<size_t> read(T *buf) {
	return read(buf, sizeof(T));
    }
    template <typename T, size_t N> result<size_t> read(T (*buf)[N]) {
	return read(buf, sizeof(T)*N);
    }
    template <typename T> result<size_t> write(const T *buf) {
	return write(buf, sizeof(T));
    }
    template <typename T, size_t N> result<size_t> write(const T (*buf)[N]) {
	return write(buf, sizeof(T)*N);
    }

    io_result read_n(void *bufaddr, size_t req_n);
    io_result write_n(const void *bufaddr, size_t req_n);
    template <typename T> io_result read_n(T *buf) {
	return read_n(buf, sizeof(T));
    }
    template <typename T, size_t N> io_result read_n(T (*buf)[N]) {
	return read_n(buf, sizeof(T)*N);
    }
    template <typename T> io_result write_n(const T *buf) {
	return write_n(buf, sizeof(T));
    }
    template <typename T, size_t N> io_result write_n(const T (*buf)[N]) {
	return write_n(buf, sizeof(T)*N);
    }

    io_result write_v(::iovec* const iov, const int& ioc) {
	::iovec *iovp = iov;
	int ioc_io = ioc;
	return write_v(iovp, ioc_io);
    }

    io_result write_v(::iovec*& iov_io, int& ioc_io);

    result<uint32_t> wait(uint32_t which, c7::usec_t tmo_us);

    result<uint32_t> wait(c7::usec_t tmo_us = -1) {
	return wait(READABLE|WRITABLE, tmo_us);
    }

    result<bool> wait_readable(c7::usec_t tmo_us = -1) {
	auto res = wait(READABLE, tmo_us);
	if (!res)  {
	    return res.as_error();
	}
	return c7result_ok((res.value() & READABLE) != 0);
    }

    result<bool> wait_writable(c7::usec_t tmo_us = -1) {
	auto res = wait(WRITABLE, tmo_us);
	if (!res) {
	    return res.as_error();
	}
	return c7result_ok((res.value() & WRITABLE) != 0);
    }

    virtual std::string to_string(const std::string& spec) const;
    virtual void print(std::ostream& out, const std::string& spec) const;

protected:
    int fdnum_ = -1;
    mutable std::string name_;

    virtual void _close() {}
};

result<fd> open(std::string&&, int oflag = 0, ::mode_t mode = 0600);
result<fd> open(int dirfd, std::string&&, int oflag = 0, ::mode_t mode = 0600);
result<fd> open(const std::string&, int oflag = 0, ::mode_t mode = 0600);
result<fd> open(int dirfd, const std::string&, int oflag = 0, ::mode_t mode = 0600);
result<fd> opentmp(const std::string& dir, ::mode_t mode = 0600);
result<std::pair<fd, fd>> make_pipe();
result<> make_pipe(fd (&fdv)[2]);

using pipe = fd;	// for source level compatibility


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7fd.hpp
