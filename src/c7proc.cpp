/*
 * c7proc.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include "c7proc.hpp"
#include "c7signal.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <atomic>
#include <unordered_set>


namespace c7 {


namespace signal {
void __enable(void (*sigchld)(int));
}


/*----------------------------------------------------------------------------
                                  proc::impl
----------------------------------------------------------------------------*/

class proc::impl {

    // -----------------------------------
    // process finalizing (handle SIGCHLD)
    // -----------------------------------
private:
    static std::unordered_set<std::shared_ptr<proc::impl>> procs;

    bool try_wait() {
	auto defer = cv_.lock();
	if (pid_ != -1) {
	    int status;
	    int wait_ret = ::waitpid(pid_, &status, WNOHANG);
	    if (wait_ret == pid_) {
		pid_ = -1;
		if (WIFEXITED(status)) {
		    state_ = EXIT;
		    value_ = WEXITSTATUS(status);
		} else {
		    state_ = KILLED;
		    value_ = WTERMSIG(status);
		}
		cv_.notify_all();
		return true;
	    }
	}
	return false;
    }

    static void waitprocs(int) {
	// range-for DON'T work well
	auto cur = procs.begin();
	auto end = procs.end();
	while (cur != end) {
	    auto p = cur++;
	    if ((*p)->try_wait()) {
		(*p)->on_finish(proc::proxy(*p));
		procs.erase(p);
	    }
	}
    }

    // -------------------------
    // proc::impl implementaion
    // -------------------------
private:

    static std::atomic<uint64_t> id_counter_;

    c7::thread::condvar cv_;
    uint64_t id_;
    ::pid_t pid_ = -1;
    state_t state_ = IDLE;
    int value_ = 0;		// EXIT:exit value, KILLED:signal number

    std::string prog_;
    c7::strvec argv_;

public:
    c7::delegate<void, proc::proxy> on_start;
    c7::delegate<void, proc::proxy> on_finish;

    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;

    impl(): id_(++id_counter_) {
	c7::signal::__enable(waitprocs);
    };

    result<void> start(const std::string& prog,
		       const c7::strvec& argv,
		       std::function<bool()> preexec,
		       std::shared_ptr<proc::impl> self,
		       int conf_fd);

    void manage(pid_t pid,
		const std::string& prog,
		std::shared_ptr<proc::impl> self);

    result<void> kill(int sig) {
	auto defer = cv_.lock();
	if (state_ == EXIT || state_ == KILLED)
	    return c7result_ok();
	if (pid_ != -1) {
	    if (::kill(pid_, sig) == C7_SYSOK)
		return c7result_ok();
	}  else {
	    errno = 0;
	}
	return c7result_err(errno, "kill failed: %{}", format());
    }

    result<void> wait() {
	auto defer = cv_.lock();
	while (state_ == RUNNING)
	    cv_.wait();
	if (state_ == EXIT || state_ == KILLED)
	    return c7result_ok();
	return c7result_err("process has not been run: %{}", format());
    }

    std::pair<state_t, int> state() {
	return std::make_pair(state_, value_);
    }

    uint64_t id() {
	return id_;
    }

    std::string format(const std::string& format_str = "") {
	static const char *state_list[] = {
	    "IDLE", "FAILED", "RUNNIG", "EXIT", "KILLED"
	};
	const char *state_str = "?";
	if (0 <= state_ && state_ < c7_numberof(state_list))
	    state_str = state_list[state_];
	return c7::format("proc#%{}<%{},pid:%{},%{},%{}>",
			  id_, prog_, pid_, state_str, value_);
    }
};

std::unordered_set<std::shared_ptr<proc::impl>> proc::impl::procs;
std::atomic<uint64_t> proc::impl::id_counter_;


static result<void> fork_x(pid_t& newpid, int& chkpipe)
{
    int pipefd[2];
    if (::pipe(pipefd) == C7_SYSERR) {
	return c7result_err(errno, "pipe failed");
    }

    if ((newpid = ::fork()) == C7_SYSERR) {
	(void)::close(pipefd[0]);
	(void)::close(pipefd[1]);
	return c7result_err(errno, "fork failed");
    }

    (void)::close(pipefd[newpid != 0]);
    chkpipe = pipefd[newpid == 0];
    return c7result_ok();
}

static bool fd_renumber(int& fd_io, int target_fd)
{
    if (fd_io == target_fd)
	return true;
    (void)::close(target_fd);
    if (::fcntl(fd_io, F_DUPFD, target_fd) != C7_SYSERR) {
	(void)close(fd_io);
	fd_io = target_fd;
	return true;
    }
    return false;
}

static bool fd_setcloexec(int fd)
{
    return (::fcntl(fd, F_SETFD, FD_CLOEXEC) == C7_SYSOK);
}

static result<pid_t> forkexec(int conf_fd,
			      std::function<bool()> preexec,
			      const std::string& prog,
			      const c7::strvec& argv)
{
    pid_t newpid;
    int chkpipe = -1;
    char errval;

    if (auto res = fork_x(newpid, chkpipe); !res)
	return res;

    if (newpid == 0) {
	//
	// child process
	//
	if (fd_renumber(chkpipe, conf_fd) && fd_setcloexec(chkpipe)) {
	    if (!preexec || preexec()) {
		// DON'T INHERIT signal mask
		::sigset_t sigs;
		::sigemptyset(&sigs);

		// [CAUTION]
		// context of c7::signal is inherited from parent process and
		// it indicate that monitor thread is running but there is no one,
		// because threads are not inherited to child process. So that 
		// means we cannot use API of c7::signal on forked process.
		////c7::signal::set(sigs);
		(void)::sigprocmask(SIG_SETMASK, &sigs, nullptr);

		// vector<string> -> vector<char *> with terminal null pointer
		auto c_av = argv.c_strv();

		// if execvp() succeed, chkpipe is closed and
		// read() on parent process return 0 (nothing data).
		::execvp(prog.c_str(), c_av.data());
	    }
	}
	// some error happen: send errno to parent process.
	// read() on parent process return 1 (errno value).
	errval = errno;
	c7::drop = ::write(chkpipe, &errval, 1);
	_exit(EXIT_FAILURE);	// don't use exit to prevent side-effect of stdio
    }

    //
    // parent process
    //
    int n = ::read(chkpipe, &errval, 1);
    (void)::close(chkpipe);
    if (n == 1) {
	// error number received
	(void)::waitpid(newpid, 0, 0);
	return c7result_err(errval, "preexec() or execvp() failed");
    }

    int status;
    if (::waitpid(newpid, &status, WNOHANG) == newpid) {
	// preexec() maybe crached. (or program exit or crashed soon after run).
	if (WIFEXITED(status))
	    return c7result_err("program maybe exit soon: exit:%{}", WEXITSTATUS(status));
	else
	    return c7result_err("preexec() maybe crashed: signal:%{}", WTERMSIG(status));
    }

    return c7result_ok(newpid);
}

result<void>
proc::impl::start(const std::string& prog,
		  const c7::strvec& argv,
		  std::function<bool()> preexec,
		  std::shared_ptr<proc::impl> self,
		  int conf_fd)
{
    if (state_ != IDLE)
	return c7result_err("proc object is in use: %{}", format());

    prog_ = prog;
    argv_ = argv;

    auto res = forkexec(conf_fd, preexec, prog_, argv_);
    if (!res) {
	state_ = FAILED;
	return c7result_err(std::move(res), "proc::start failed: %{}", format());
    }

    {
	auto unload_defere = cv_.lock();
	pid_ = res.value();
	state_ = RUNNING;
	procs.insert(self);
    }

    // To shorten time of locking cv_, on_start() is called after unlock cv_. 
    on_start(proc::proxy(self));

    // If forked process exit immediately before process informaton is registered
    // to procs at above code block, SIGCHLD handler already has been called, but
    // it cannot update state of this process. So, next calling kill() is needed to
    // prevent caller's proc::wait() from block.
    (void)::kill(::getpid(), SIGCHLD);

    return c7result_ok();
}


void
proc::impl::manage(pid_t pid,
		   const std::string& prog,
		   std::shared_ptr<proc::impl> self)
{
    prog_ = prog;
    argv_ = c7::strvec{prog_};

    auto unload_defere = cv_.lock();
    pid_ = pid;
    state_ = RUNNING;
    procs.insert(self);
    (void)::kill(::getpid(), SIGCHLD);	// this is needed by same reason of above.
}


/*----------------------------------------------------------------------------
                                     proc
----------------------------------------------------------------------------*/

proc::proc():
    pimpl(new proc::impl()),
    on_start(pimpl->on_start), on_finish(pimpl->on_finish) {
}

result<void> proc::_start(const std::string& program,
			  const c7::strvec& argv,
			  std::function<bool()> preexec)
			  
{
    return pimpl->start(program, argv, preexec, pimpl, conf_fd);
}

void proc::manage_external(pid_t pid, const std::string& program)
{
    pimpl->manage(pid, program, pimpl);
}

result<void> proc::signal(int sig)
{
    return pimpl->kill(sig);
}

result<void> proc::wait()
{
    return pimpl->wait();
}

std::pair<proc::state_t, int> proc::state() const
{
    return pimpl->state();
}

uint64_t proc::id() const
{
    return pimpl->id();
}

std::string proc::format(const std::string& format_str) const
{
    return pimpl->format(format_str);
}


/*----------------------------------------------------------------------------
                                 proxy::proc
----------------------------------------------------------------------------*/

proc::proxy::proxy(std::shared_ptr<proc::impl> p): pimpl(p) {}

std::pair<proc::state_t, int> proc::proxy::state() const
{
    return pimpl->state();
}

uint64_t proc::proxy::id() const
{
    return pimpl->id();
}

std::string proc::proxy::format(const std::string& format_str) const
{
    return pimpl->format(format_str);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7
