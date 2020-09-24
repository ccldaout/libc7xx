/*
 * c7file.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cstring>
#include <c7file.hpp>
#include <c7path.hpp>
#include <c7signal.hpp>
#include <c7string.hpp>


namespace c7 {
namespace file {


/*----------------------------------------------------------------------------
                            file mode / owner /...
----------------------------------------------------------------------------*/

result<void> fchstat(int fd, const std::string& ref_path)
{
    struct ::stat st;
    if (::stat(ref_path.c_str(), &st) == C7_SYSERR)
	return c7result_err(errno, "stat failed: %{}", ref_path);

    if (::fchmod(fd, st.st_mode) == C7_SYSERR)
	return c7result_err(errno, "fchmod failed");

    if (geteuid() == 0 && ::fchown(fd, st.st_uid, st.st_gid) == C7_SYSERR)
	return c7result_err(errno, "fchown(%{}, %{}) failed (on root)", st.st_uid, st.st_gid);

    return c7result_ok();
}

c7::result<void> inherit_owner(const std::string& path)
{
    std::string dir(path, 0, c7path_name(path.c_str()) - path.c_str());

    struct ::stat st;
    if (::stat(dir.c_str(), &st) == C7_SYSERR)
	return c7result_err(errno, "stat failed: %{}", dir);

    if (::chown(path.c_str(), st.st_uid, st.st_gid) == C7_SYSERR)
	return c7result_err(errno, "chown failed: %{} %{} %{}", path, st.st_uid, st.st_gid);

    return c7result_ok();
}


/*----------------------------------------------------------------------------
                               recursive mkdir
----------------------------------------------------------------------------*/

struct mkdir_prm {
    uid_t uid;
    gid_t gid;
    mode_t mode;
    char *path;
};

static result<void> mkdir_x(mkdir_prm& prm)
{
    if (::mkdir(prm.path, prm.mode) == C7_SYSOK) {
	if (::chown(prm.path, prm.uid, prm.gid) == C7_SYSOK) {
	    return c7result_ok();
	}
	return c7result_err(errno, "chown failed: %{}", prm.path);
    }
    if (errno == EEXIST) {
	return c7result_ok();
    }
    return c7result_err(errno, "mkdir failed: %{}", prm.path);
}

static result<void> stepmkdir(char *namepos, mkdir_prm &prm)
{
    for (;;) {
	if (namepos[0] == 0) {
	    return c7result_ok();
	}
	auto nextslash = std::strchr(namepos, '/');
	if (nextslash == nullptr) {
	    return mkdir_x(prm);
	}
	nextslash[0] = 0;
	if (auto res = mkdir_x(prm); !res) {
	    return res;
	}
	nextslash[0] = '/';
	namepos = nextslash + 1;
    }
}

c7::result<void> mkdir(const std::string& path, mode_t mode, uid_t uid, gid_t gid)
{
    if (uid == -1U)
	uid = geteuid();
    if (gid == -1U)
	gid = getegid();

    char *buf = (char *)std::malloc(path.size() + 1);
    if (buf == nullptr)
	return c7result_err(errno, "cannot allocate working memory: %{}", path.size()+1);
    auto defer = c7::defer(std::free, buf);
    (void)std::strcpy(buf, path.c_str());

    mkdir_prm prm = { .uid = uid, .gid = gid, .mode = mode, .path = buf };

    return stepmkdir((buf[0] == '/') ? &buf[1] : buf, prm);
}


/*----------------------------------------------------------------------------
                              write entire file
----------------------------------------------------------------------------*/

c7::result<void> write(const std::string& path, mode_t mode, const void *buf, size_t size)
{
    int fd = ::open(path.c_str(), O_WRONLY|O_CREAT|O_TRUNC, mode);
    if (fd == C7_SYSERR)
	return c7result_err(errno, "open failed: %{}", path);
    auto closer = c7::defer(::close, fd);

    ssize_t actsize = ::write(fd, buf, size);
    if (actsize != (ssize_t)size)
	return c7result_err(errno,
			    "write failed: %{}, req:%{}, act:%{}", path, size, actsize);

    return c7result_ok();
}


/*----------------------------------------------------------------------------
                             rewrite entire file
----------------------------------------------------------------------------*/

template <typename Action>
static result<std::string> trytmp(const std::string& ref_path, Action action)
{
    size_t fixsize = c7path_name(ref_path.c_str()) - ref_path.c_str();
    std::string tmppath(ref_path, fixsize);
    tmppath += ".tmp.";
    fixsize = tmppath.size();

    for (uint64_t u = 1; u != 0; u++) {
	tmppath.replace(fixsize, std::string::npos, std::to_string(u));
	int err = action(tmppath);
	if (err == 0)
	    return c7result_ok(std::move(tmppath));
	if (err != EEXIST)
	    return c7result_err(errno, "action failed: %{}", tmppath.c_str());
    }

    return c7result_err("cannot ready temporary file: ref_path: %{}", ref_path);
}

static result<std::string> reservetmp(const std::string& ref_path)
{
    return trytmp(ref_path,
		  [&ref_path](const std::string& candidate) {
		      int fd = ::open(candidate.c_str(), O_WRONLY|O_CREAT|O_EXCL, 0600);
		      if (fd == C7_SYSERR)
			  return errno;
		      (void)fchstat(fd, ref_path);
		      (void)close(fd);
		      return 0;
		  });
}

static result<std::string> linktmp(const std::string& ref_path)
{
    return trytmp(ref_path,
		  [&ref_path](const std::string& candidate) {
		      if (::link(ref_path.c_str(), candidate.c_str()) == C7_SYSERR)
			  return errno;
		      return 0;
		  });
}

static int unlink_if(const char *path)
{
    struct ::stat st;
    if (::stat(path, &st) == C7_SYSOK && st.st_nlink == 1)
	return ::unlink(path);
    return 0;
}

c7::result<void> rewrite(const std::string& path, void *buf, size_t size)
{
    auto res = reservetmp(path);
    if (!res)
	return c7result_err(std::move(res), "rewrite failed");
    auto tmppath = res.value();
    auto rmvtmp = c7::defer(::unlink, tmppath.c_str());

    if (auto res = c7::file::write(tmppath, 0600, buf, size); !res)
	return c7result_err(std::move(res), "rewrite failed");

    if (res = linktmp(path); !res)
	return c7result_err(std::move(res), "rewrite failed");
    auto bckpath = res.value();
    auto rmvbck = c7::defer(unlink_if, bckpath.c_str());
    
    auto unblock_defer = c7::signal::block();
    auto lastret = c7result_ok();
    if (::rename(tmppath.c_str(), path.c_str()) == C7_SYSERR) {
	::unlink(bckpath.c_str());
	c7result_seterr(lastret, "rewrite failed: rename: %{} -> %{}", tmppath, path);
    }

    return lastret;
}

/*----------------------------------------------------------------------------
                              read partial file
----------------------------------------------------------------------------*/

static const char *path_s(const std::string& path)
{
    if (path.empty() || path == "-")
	return "<stdin>";
    return path.c_str();
}

static result<int> ropen(const std::string& path)
{
    if (path.empty() || path == "-") {
	int fd = ::dup(0);
	if (fd == C7_SYSERR) {
	    return c7result_err(errno, "dup failed");
	}
	return c7result_ok(fd);
    }

    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd == C7_SYSERR) {
	return c7result_err(errno, "open failed: %{}", path);
    }
    return c7result_ok(fd);
}

c7::result<ssize_t> read_into(const std::string& path, void *buf, size_t size)
{
    auto res = ropen(path);
    if (!res) {
	return res;
    }
    auto fd = res.value();
    ssize_t az = ::read(fd, buf, size);
    (void)close(fd);
    if (az == -1) {
	return c7result_err(errno, "read failed: %{}", path_s(path));
    }
    return c7result_ok((ssize_t)(size - az));
}


/*----------------------------------------------------------------------------
                               read entire file
----------------------------------------------------------------------------*/

static result<int> rx_open(const std::string& path, struct ::stat *st)
{
    auto res = ropen(path);
    if (!res)
	return res;
    int fd = res.value();
    auto closer = c7::defer(::close, fd);

    if (::fstat(fd, st) == C7_SYSERR)
	return c7result_err(errno, "fstat failed: fd:%d, %{}", fd, path_s(path));

    closer.cancel();
    return res;
}

static result<char*> rx_once(int fd, ssize_t rz)
{
    char *p = (char *)std::malloc(rz + 1);
    if (p != nullptr) {
	ssize_t az;
	if ((az = ::read(fd, p, rz)) == rz) {
	    p[rz] = 0;		/* for text file */
	} else {
	    free(p);
	    return c7result_err(az == C7_SYSERR ? errno : EIO,
				"read failed: req:%{}, act:%{}", rz, az);
	}
    }
    return c7result_ok(p);
}

static result<char*> rx_repeat(int fd, size_t& size)
{
    const int xz = 16 * 1024;
    char *tp = nullptr;
    char *ep = tp;
    char *cp = tp;

    // tp will be changed at realloc, so it must be captured by reference.
    auto free_on_error = c7::defer([tp](){ std::free(tp); });

    for (;;) {
	if (ep == cp) {
	    char *new_p = (char *)std::realloc(tp, (size = cp - tp) + xz);
	    if (new_p == nullptr) {
		return c7result_err(errno, "realloc failed: req:%{}", size + xz);
	    }
	    ep = (cp = (tp = new_p) + size) + xz;
	}

	size = ep - cp;
	ssize_t rn = ::read(fd, cp, size);
	if (rn == C7_SYSERR) {
	    return c7result_err(errno, "read failed");
	}
	if (rn == 0) {
	    *cp = 0;		// null charater for text access
	    size = cp - tp;
	    char *new_p = (char *)std::realloc(tp, size + 1);	// +1: last added null character
	    if (new_p == nullptr) {
		return c7result_err(errno, "realloc failed: req:%{}", size + 1);
	    }		
	    free_on_error.cancel();
	    return c7result_ok(new_p);			// success
	}
	cp += rn;
    }
}

result<void*> read_impl(const std::string& path, size_t& size)
{
    struct ::stat st;
    auto o_res = rx_open(path, &st);
    if (!o_res)
	return o_res;

    int fd = o_res.value();
    auto closer = c7::defer(::close, fd);

    result<char*> r_res;

    if (S_ISREG(st.st_mode)) {
	r_res = rx_once(fd, st.st_size);
    } else {
	r_res = rx_repeat(fd, size);
    }
    if (!r_res) {
	return c7result_err(std::move(r_res), "read_impl failed: %{}", path);
    }
    return c7result_ok(static_cast<void*>(r_res.value()));
}


/*----------------------------------------------------------------------------
                            read entire text file
----------------------------------------------------------------------------*/

result<std::vector<std::string>> readlines(const std::string& path)
{
    auto res = read<char>(path);
    if (!res)
	return res;
    return c7result_ok(c7::str::split(res.value().get(), '\n'));
}


/*----------------------------------------------------------------------------
                         convenience mmap operations
----------------------------------------------------------------------------*/

static result<void*> dommap_fd(const std::string& path, int fd, int prot, size_t& size_io)
{
    struct ::stat sbuf;

    if (::fstat(fd, &sbuf) == C7_SYSERR) {
	return c7result_err(errno, "fstat failed: %{}", path);
    }

    if (size_io == 0) {
	size_io = sbuf.st_size;
    } else if (size_io > static_cast<size_t>(sbuf.st_size)) {
	if (::ftruncate(fd, size_io) == C7_SYSERR) {
	    return c7result_err(errno, "ftruncate failed: %{}, size:%{}", path, size_io);
	}
    }

    int mflag = MAP_SHARED;
    auto addr = ::mmap(nullptr, size_io, prot, mflag, fd, 0); 
    if (addr != (void *)C7_SYSERR) {
	return c7result_ok(addr);
    }

    return c7result_err(errno, "mmap failed: %{}, size:%{}", path, size_io);
}

result<void*> mmap_impl(const std::string& path, size_t& size_io, int oflag)
{
    int fd = open(path.c_str(), oflag, 0600);
    if (fd == C7_SYSERR) {
	return c7result_err(errno, "open failed: %{}", path);
    }
    auto defer = c7::defer(::close, fd);

    if ((oflag & O_CREAT) != 0) {
	(void)inherit_owner(path);
    }
    
    int prot = PROT_READ;
    if ((oflag & O_RDWR) != 0)
	prot |= PROT_WRITE;

    return dommap_fd(path, fd, prot, size_io);
}

result<void*> mmap_impl(int fd, size_t& size_io, int oflag)
{
    int prot = PROT_READ;
    if ((oflag & O_RDWR) != 0) {
	prot |= PROT_WRITE;
    }
    return dommap_fd(c7::format("<fd:%{}>", fd), fd, prot, size_io);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace file
} // namespace c7
