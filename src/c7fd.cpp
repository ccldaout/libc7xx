/*
 * c7fd.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7fd.hpp>
#include <c7utils.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>


namespace c7 {


/*----------------------------------------------------------------------------
                                  I/O result
----------------------------------------------------------------------------*/

std::string io_result::to_string(status status)
{
    const char *sv[] = { "OK", "BUSY", "CLOSED", "INCOMP", "ERR" };
    int i = static_cast<int>(status);
    const char *p;
    if (0 <= i && i < c7_numberof(sv))
	p = sv[i];
    else
	p = "???";
    return c7::format("%{}(%{})", p, i);
}

std::string io_result::to_string(const std::string& spec) const
{
    if (status_ == status::OK || result_)
	return c7::format("io_result<%{},done:%{},remain:%{}>",
			  to_string(status_), done_, remain_);
    return c7::format("io_result<%{},done:%{},remain:%{}>:error: %{}",
		      to_string(status_), done_, remain_, result_);
}

void io_result::print(std::ostream& out, const std::string& spec) const
{
    out << to_string(spec);
}

void print_type(std::ostream& out, const std::string& format, io_result::status arg)
{
    out << io_result::to_string(arg);
}


/*----------------------------------------------------------------------------
                                      fd
----------------------------------------------------------------------------*/

fd::fd() {}

fd::fd(int fdnum, const std::string& name):
    fdnum_(fdnum), name_(name)
{
}

fd::fd(fd&& o):
    on_close(std::move(o.on_close)), fdnum_(o.fdnum_), name_(std::move(o.name_)) 
{
    o.fdnum_ = -1;
}

fd& fd::operator=(fd&& o)
{
    if (this != &o) {
	close();
	on_close = std::move(o.on_close);
	fdnum_   = o.fdnum_;
	name_    = std::move(o.name_);
	o.fdnum_  = -1;
    }
    return *this;
}

result<> fd::close()
{
    if (fdnum_ != -1) {
	auto _on_close = std::move(on_close);
	_on_close(*this);
	if (fdnum_ != -1) {
	    _close();
	    int fd = fdnum_;
	    fdnum_ = -1;
	    if (::close(fd) == C7_SYSERR) {
		return c7result_err(errno, "close(%{}) failed", fd);
	    }
	    name_ += c7::format(":CLOSED<%{}>", fd);
	}
    }
    return c7result_ok();
}

result<> fd::renumber_to(int target_fdnum)
{
    if (fdnum_ != target_fdnum) {
	(void)::close(target_fdnum);
	return fd::renumber_above(target_fdnum);
    }
    return c7result_ok();
}

result<> fd::renumber_above(int lowest_fdnum)
{
    int newfd = ::fcntl(fdnum_, F_DUPFD, lowest_fdnum);
    if (newfd == C7_SYSERR) {
	return c7result_err(errno, "fcntl(%{}, F_DUPFD, %{}) failed", fdnum_, lowest_fdnum);
    }
    (void)::close(fdnum_);
    fdnum_ = newfd;
    return c7result_ok();
}

result<int> fd::get_flag(int flagtype)
{
    int ret = ::fcntl(fdnum_, flagtype, 0);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "fcntl(%{}, %{})", fdnum_, flagtype);
    }
    return c7result_ok(ret);
}

result<> fd::set_flag(int flagtype, int value)
{
    int ret = ::fcntl(fdnum_, flagtype, value);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "fcntl(%{}, %{}, %{})", fdnum_, flagtype, value);
    }
    return c7result_ok();
}

result<> fd::change_flag(int set_flagtype, int flags, bool on)
{
    int get_flagtype;
    switch (set_flagtype) {
      case F_SETFD:
	get_flagtype = F_GETFD;
	break;

      case F_SETFL:
	get_flagtype = F_GETFL;
	break;

      default:	
	return c7result_err(EINVAL, "Unsupported flag type: %{}", set_flagtype);
    }

    auto ret = get_flag(get_flagtype);
    if (!ret) {
	return ret.as_error();
    }
    
    int newflag = ret.value();
    if (on) {
	newflag |= flags;
    } else {
	newflag &= ~flags;
    }

    return set_flag(set_flagtype, newflag);
}

result<bool> fd::get_cloexec()
{
    auto ret = get_flag(F_GETFD);
    if (!ret) {
	return ret.as_error();
    }
    return c7result_ok((ret.value() & FD_CLOEXEC) != 0);
}

result<> fd::set_cloexec(bool enable)
{
    return change_flag(F_SETFD, FD_CLOEXEC, enable);
}

result<bool> fd::get_nonblocking()
{
    auto ret = get_flag(F_GETFL);
    if (!ret) {
	return ret.as_error();
    }
    return c7result_ok((ret.value() & O_NONBLOCK) != 0);
}

result<> fd::set_nonblocking(bool enable)
{
    return change_flag(F_SETFL, O_NONBLOCK, enable);
}

result<> fd::chmod(::mode_t mode)
{
    int ret = ::fchmod(fdnum_, mode);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "fchmod(%{}, %{o}) failed", fdnum_, mode);
    }
    return c7result_ok();
}

result<> fd::chown(uid_t uid)
{
    return fd::chown(uid, getegid());
}

result<> fd::chown(uid_t uid, gid_t gid)
{
    int ret = ::fchown(fdnum_, uid, gid);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "fchown(%{}, %{}, %{}) failed", fdnum_, uid, gid);
    }
    return c7result_ok();
}

result<> fd::truncate(size_t size)
{
    int ret = ::ftruncate(fdnum_, size);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "ftruncate(%{}, %{}) failed", fdnum_, size);
    }
    return c7result_ok();
}

result<fd::stat_t> fd::stat() const
{
    fd::stat_t st;
    int ret = ::fstat(fdnum_, &st);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "fstat(%{})", fdnum_);
    }
    return c7result_ok(st);
}

result<off_t> fd::seek_abs(off_t offset)
{
    off_t ret = ::lseek(fdnum_, offset, SEEK_SET);
    if (ret == (off_t)C7_SYSERR) {
	return c7result_err(errno, "lseek(%{}, %{}, SEEK_SET)", fdnum_, offset);
    }
    return c7result_ok(ret);
}

result<off_t> fd::seek_cur(off_t offset)
{
    off_t ret = ::lseek(fdnum_, offset, SEEK_CUR);
    if (ret == (off_t)C7_SYSERR) {
	return c7result_err(errno, "lseek(%{}, %{}, SEEK_CUR)", fdnum_, offset);
    }
    return c7result_ok(ret);
}

result<size_t> fd::read(void *bufaddr, size_t size)
{
    ssize_t ret = ::read(fdnum_, bufaddr, size);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "read(%{}, %{}) failed", fdnum_, size);
    }
    return c7result_ok((size_t)ret);
}

result<size_t> fd::write(const void *bufaddr, size_t size)
{
    ssize_t ret = ::write(fdnum_, bufaddr, size);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "write(%{}, %{}) failed", fdnum_, size);
    }
    return c7result_ok((size_t)ret);
}

io_result fd::read_n(void *buf, size_t const req_n)
{
    size_t n = 0;	// actual read bytes

    while (n != req_n) {
	ssize_t z = ::read(fdnum_, buf, (req_n - n));
	if (z <= 0) {
	    auto status = io_result::status::ERR;
	    const char *descrip = "error";
	    
	    if (z == 0 && n == 0) {
		status = io_result::status::CLOSED;
		descrip = "maybe closed";
		errno = ENODATA;
	    } else if (z == 0) {
		// 0 < n < req_n
		status = io_result::status::INCOMP;
		descrip = "maybe closed";
		errno = ENODATA;
	    } else if (errno == EWOULDBLOCK) {
 		// z == C7_SYSERR
		status = io_result::status::BUSY;
		descrip = "busy";
	    } else {
		// z == C7_SYSERR
	    }
	    return io_result(status, n, req_n - n,
			     c7result_err(errno,
					  "read_n(%{}, %{}) -> %{} (%{})",
					  *this, req_n, n, descrip));
	}
	buf = (char *)buf + z;
	n += z;
    }
    return io_result::ok(n);
}

io_result fd::write_n(const void *buf, size_t const req_n)
{
    size_t n = 0;	// actual writen bytes

    while (n != req_n) {
	ssize_t z = ::write(fdnum_, buf, (req_n - n));
	if (z <= 0) {
	    if (errno == EWOULDBLOCK) {
		return io_result(io_result::status::BUSY, n, req_n - n,
				 c7result_err(errno,
					      "write_n(%{}, %{}) -> %{} (busy)",
					      *this, req_n, n));
	    } else {
		return io_result(io_result::status::ERR, n, req_n - n,
				 c7result_err(errno,
					      "write_n(%{}, %{}) -> %{} (error)",
					      *this, req_n, n));
	    }
	}
	buf = (char *)buf + z;
	n += z;
    }
    return io_result::ok(n);
}

io_result fd::write_v(::iovec*& iov, int& ioc)
{
    size_t n = 0;	// actual writen bytes

    while (ioc > 0) {
	if (iov->iov_len == 0) {
	    ioc--;
	    iov++;
	    continue;
	}
	ssize_t z = ::writev(fdnum_, iov, ioc);
	if (z <= 0) {
	    for (int i = 0, z = 0; i < ioc; i++) {
		z += iov[i].iov_len;
	    }
	    if (errno == EWOULDBLOCK) {
		return io_result(io_result::status::BUSY, n, z,
				 c7result_err(errno,
					      "write_v(%{}) (busy)", *this));
	    } else {
		return io_result(io_result::status::ERR, n, z,
				 c7result_err(errno,
					      "write_v(%{}) (error)", *this));
	    }
	}
	n += z;
	while (z > 0) {
	    if (iov->iov_len <= (size_t)z) {
		z -= iov->iov_len;
		iov->iov_len = 0;
		iov->iov_base = nullptr;
		iov++;
		ioc--;
	    } else if (iov->iov_len > (size_t)z) {
		iov->iov_len -= z;
		iov->iov_base = (char *)iov->iov_base + z;
		break;
	    }
	}
    }
    return io_result::ok(n);
}

result<uint32_t> fd::wait(uint32_t which, c7::usec_t tmo_us)
{
    ::pollfd pfd;

    pfd.fd = fdnum_;

    pfd.events = 0;
    if ((which & fd::READABLE) != 0) {
	pfd.events |= POLLIN|POLLRDHUP;
    }
    if ((which & fd::WRITABLE) != 0) {
	pfd.events |= POLLOUT;
    }

    int tmo_ms = (tmo_us + 999) / 1000;		// Oops !

    int ret = ::poll(&pfd, 1, tmo_ms);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "poll(%{}) failed", *this);
    }

    if (ret == 0) {
	return c7result_ok(0U);			// timeout 
    }

    which = 0;
    if ((pfd.revents & (POLLIN|POLLRDHUP|POLLERR|POLLHUP)) != 0) {
	which |= fd::READABLE;
    }
    if ((pfd.revents & (POLLOUT|POLLERR|POLLHUP)) != 0) {
	which |= fd::WRITABLE;
    }
    return c7result_ok(which);
}

std::string fd::to_string(const std::string&) const
{
    if (name_.empty())
	return c7::format("fd<%{}>", fdnum_);
    else
	return c7::format("fd<%{},%{}>", fdnum_, name_);
}

void fd::print(std::ostream& out, const std::string& spec) const
{
    out << to_string(spec);
}

result<fd> open(std::string&& path, int oflag, ::mode_t mode)
{
    int ret = ::open(path.c_str(), oflag, mode);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "open(%{}, %{#o}, %{#o}) failed", path, oflag, mode);
    }	
    return c7result_ok(fd(ret, std::move(path)));
}

result<fd> open(const std::string& path, int oflag, ::mode_t mode)
{
    return open(std::string(path), oflag, mode);
}

result<fd> opentmp(const std::string& dir, ::mode_t mode)
{
#if 0
    int ret = ::open(dir.c_str(), O_TMPFILE|O_RDWR, mode);
    if (ret == C7_SYSERR) {
	return c7result_err(errno, "open(%{}, O_TMPFILE|O_RDWR, %{#o}) failed", dir, mode);
    }	
    return c7result_ok(fd(ret, dir + "/<tmporary>"));
#else
# if defined(O_PATH)
    int oflag = O_PATH;
# else
    int oflag = O_RDONLY;
# endif
    int dirfd = ::open(dir.c_str(), oflag);
    if (dirfd == C7_SYSERR) {
	return c7result_err(errno, "open(%{}, O_PATH) failed", dir);
    }	
    auto defer = c7::defer(::close, dirfd);

    auto pid = ::getpid();
    for (uint16_t u = 1; u != 0; u++) {
	auto name = c7::format(".c7.%{}.%{}.%{}", pid, c7::time_us(), u);
	int ret = ::openat(dirfd, name.c_str(), O_CREAT|O_RDWR|O_EXCL, mode);
	if (ret != C7_SYSERR) {
	    (void)::unlinkat(dirfd, name.c_str(), 0);
	    return c7result_ok(fd(ret, dir + "/<tmporary>"));
	}
	if (errno != EEXIST) {
	    break;
	}
    }
    return c7result_err(errno, "opentmp(%{}) failed", dir);

#endif
}

result<std::pair<fd, fd>> make_pipe()
{
    static std::atomic<size_t> id_counter;
    int fdv[2];
    if (::pipe(fdv) == C7_SYSERR) {
	return c7result_err(errno, "pipe() failed");
    }	
    id_counter++;
    return c7result_ok(std::make_pair(c7::fd(fdv[0], c7::format("R.%{}", id_counter)),
				      c7::fd(fdv[1], c7::format("W.%{}", id_counter))));
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7
