/*
 * c7pipeline.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <c7defer.hpp>
#include <c7pipeline.hpp>
#include <c7utils.hpp>
#include <c7nseq/reverse.hpp>


namespace c7 {


class pipeline::impl {
public:
    ~impl() {
	kill(SIGKILL);
    }

    void env(const std::string& envval) {
	envs_ += envval;
    }

    void add(const std::string& workdir,
	     const std::string& prog,
	     const c7::strvec& argv,
	     const c7::strvec& envs) {
	auto menvs = envs_;
	menvs += envs;
	procs_.emplace_back(proc_attrib{
		workdir,
		prog.empty() ? argv[0] : prog,
		argv,
		menvs,
		C7_SYSERR, -1});
    }

    c7::result<> build(int fd0, int fd1, int fd2);

    void kill(int sig) {
	for (auto& proc: procs_) {
	    if (proc.pid != C7_SYSERR) {
		::kill(proc.pid, sig);
	    }
	}
    }

    std::vector<int> wait();

private:
    struct proc_attrib {
	std::string workdir;
	std::string prog;
	c7::strvec argv;
	c7::strvec envs;
	int pid;
	int status;
    };

    c7::strvec envs_;
    std::vector<proc_attrib> procs_;

    [[noreturn]] void exec_on_child(int fd0, int fd1, int fd2, proc_attrib& proc);
};


static c7::result<>
fd_renumber_above(int& fd, int lowest_fd)
{
    int newfd = ::fcntl(fd, F_DUPFD, lowest_fd);
    if (newfd == C7_SYSERR) {
	return c7result_err(errno, "fcntl(%{}, F_DUPFD, %{}) failed", fd, lowest_fd);
    }
    ::close(fd);
    fd = newfd;
    return c7result_ok();
}


/*----------------------------------------------------------------------------
                                pipeline:impl
----------------------------------------------------------------------------*/

void
pipeline::impl::exec_on_child(int p0, int p1, int fd2, proc_attrib& proc)
{
    // reset signal settings
    ::sigset_t mask;
    ::sigemptyset(&mask);
    ::sigprocmask(SIG_SETMASK, &mask, nullptr);
    ::signal(SIGPIPE, SIG_DFL);

    if (!proc.workdir.empty()) {
	if (::chdir(proc.workdir.c_str()) == C7_SYSERR) {
	    ::exit(errno);
	}
    }

    // connect pipe
    c7::result<> res;
    res << fd_renumber_above(p0, 3)
	<< fd_renumber_above(p1, 3)
	<< fd_renumber_above(fd2, 3);
    if (!res) {
	::exit(errno);
    }
    ::close(0);
    ::close(1);
    ::close(2);
    c7::drop = dup(p0);
    c7::drop = dup(p1);
    c7::drop = dup(fd2);
    ::close(p0);
    ::close(p1);
    ::close(fd2);

    // close garbage file desriptors
    for (int i = 3; i < 256; i++) {
	::close(i);
    }

    // vector<string> -> vector<char*> with terminal null pointer
    auto c_av = proc.argv.c_strv();
    auto c_ev = proc.envs.c_strv();

    if (proc.envs.empty()) {
	::execvp(proc.prog.c_str(), c_av.data());
    } else {
	::execvpe(proc.prog.c_str(), c_av.data(), c_ev.data());
    }

    ::exit(errno);
}


c7::result<>
pipeline::impl::build(int fd0, int fd1, int fd2)
{
    int p0 = -1;
    int p1 = -1;
    int pp[2] = {-1, -1};

    c7::defer close_tmpfds(
	[&p0, &p1, &fd0, &fd1, &pp](){
	    ::close(p0);
	    ::close(p1);
	    ::close(fd0);
	    ::close(fd1);
	    ::close(pp[0]);
	    ::close(pp[1]);
	});

    std::swap(p1, fd1);

    {
	c7::defer killall(
	    [this]() {
		this->kill(SIGINT);
		c7::sleep_ms(1);
		this->kill(SIGTERM);
		c7::sleep_ms(1);
		this->kill(SIGKILL);
		this->wait();
	    });

	// build pipeline - proc#0 | proc#1 | ... | proc#N
	// by reverse order.

	for (auto& proc: procs_ | c7::nseq::reverse()) {
	    if (&proc != &procs_[0]) {
		// proc#N, proc#(N-1), ..., proc#1
		if (::pipe(pp) == C7_SYSERR) {
		    return c7result_err(errno, "pipe() failed");
		}
		std::swap(p0, pp[0]);
	    } else {
		// [last of loop] proc#0
		std::swap(p0, fd0);
	    }

	    proc.pid = ::fork();
	    if (proc.pid == C7_SYSERR) {
		return c7result_err(errno, "fork() failed");
	    }
	    if (proc.pid == 0) {
		exec_on_child(p0, p1, fd2, proc);
		// don't reach here
	    }

	    // parent process
	    ::close(p0), p0 = -1;
	    ::close(p1), p1 = -1;
	    std::swap(p1, pp[1]);
	}

	killall.cancel();
    }

    return c7result_ok();
}


std::vector<int>
pipeline::impl::wait()
{
    std::vector<int> statusv;

    for (auto& proc: procs_) {
	if (proc.pid != C7_SYSERR) {
	    int status;
	    if (::waitpid(proc.pid, &status, 0) == proc.pid) {
		if (WIFEXITED(status)) {
		    proc.status = WEXITSTATUS(status);
		} else {
		    proc.status = -WTERMSIG(status);
		}
	    } else {
		proc.status = ECHILD;
	    }
	    statusv.push_back(proc.status);
	    proc.pid = C7_SYSERR;
	}
    }

    return statusv;
}


/*----------------------------------------------------------------------------
                                   pipeline
----------------------------------------------------------------------------*/

pipeline::pipeline(): pimpl_(new impl)
{
}

pipeline::~pipeline() = default;

pipeline::pipeline(pipeline&&) = default;

pipeline& pipeline::operator=(pipeline&&) = default;

pipeline&
pipeline::env(const std::string& envval)
{
    pimpl_->env(envval);
    return *this;
}

pipeline&
pipeline::add(const std::string& workdir,
	      const std::string& prog,
	      const c7::strvec& argv,
	      const c7::strvec& envs)
{
    pimpl_->add(workdir, prog, argv, envs);
    return *this;
}

c7::result<>
pipeline::exec(int fd0, int fd1, int fd2)
{
    c7::defer on_return;
    if (fd0 == -1) {
	fd0 = open("/dev/null", O_RDONLY);
    }
    if (fd1 == -1) {
	fd1 = open("/dev/null", O_WRONLY);
    }
    if (fd2 == -1) {
	fd2 = open("/dev/null", O_WRONLY);
	on_return += [fd2](){ ::close(fd2); };
    }
    if (fd0 == fd1) {
	fd1 = ::dup(fd1);
	if (fd1 == C7_SYSERR) {
	    ::close(fd0);
	    return c7result_err(errno, "dup() failed");
	}
    }
    if (fd2 == fd1) {
	fd1 = ::dup(fd1);
	if (fd1 == C7_SYSERR) {
	    // DON'T close fd2
	    return c7result_err(errno, "dup() failed");
	}
    }
    return pimpl_->build(fd0, fd1, fd2);
}

c7::result<int>
pipeline::exec(int fd2)
{
    int socks[2];
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, socks) == C7_SYSERR) {
	return c7result_err(errno, "socketpair() failed");
    }
    if (auto res = exec(socks[1], socks[1], fd2); !res) {
	::close(socks[0]);
	return res.as_error();
    }
    return c7result_ok(socks[0]);
}

c7::result<>
pipeline::exec(int (&pfd)[2], int fd2)
{
    int in[2], out[2];
    if (::pipe(in) == C7_SYSERR) {
	return c7result_err(errno, "pipe() failed");
    }
    if (::pipe(out) == C7_SYSERR) {
	::close(in[0]);
	::close(in[1]);
	return c7result_err(errno, "pipe() failed");
    }
    if (auto res = exec(in[0], out[1], fd2); !res) {
	::close(in[1]);
	::close(out[0]);
	return res;
    }
    pfd[0] = out[0];
    pfd[1] = in[1];
    return c7result_ok();
}

void pipeline::kill(int sig)
{
    pimpl_->kill(sig);
}

std::vector<int> pipeline::wait()
{
    return pimpl_->wait();
}


} // namespace c7
