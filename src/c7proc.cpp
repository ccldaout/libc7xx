/*
 * c7proc.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <atomic>
#include <unordered_set>
#include <c7proc.hpp>
#include <c7signal.hpp>
#include <c7thread/condvar.hpp>
#include <c7thread/mutex.hpp>


namespace c7 {


namespace signal {
void enable_SIGCHLD(void (*sigchld)(int));
}


/*----------------------------------------------------------------------------
                                  proc::impl
----------------------------------------------------------------------------*/

static const char *state_string(proc::state_t state)
{
    static const char *state_list[] = {
	"IDLE", "FAILED", "RUNNIG", "EXIT", "KILLED"
    };
    if (0 <= state && state < c7_numberof(state_list)) {
	return state_list[state];
    }
    return "?";
}

class proc::impl: public std::enable_shared_from_this<proc::impl>  {
private:
    static std::unordered_set<std::shared_ptr<proc::impl>> procs_;
    static c7::thread::mutex procs_mutex_;
    static std::atomic<uint64_t> id_counter_;

    c7::thread::condvar cv_;
    c7::defer unguard_;
    uint64_t id_;
    ::pid_t pid_ = -1;
    state_t state_ = IDLE;
    int value_ = 0;		// EXIT:exit value, KILLED:signal number

    std::string prog_;
    c7::strvec argv_;

    bool try_wait_raw(bool from_waitprocs);
    bool try_wait(bool from_waitprocs);
    static void waitprocs(int);		// called on signal handling thread

public:
    c7::delegate<void, proc::proxy> on_start;
    c7::delegate<void, proc::proxy> on_finish;

    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;

    impl(): id_(++id_counter_) {
	c7::signal::enable_SIGCHLD(waitprocs);
    };

    defer guard_finish();

    result<> start(const std::string& prog,
		   const c7::strvec& argv,
		   std::function<bool()> preexec,
		   int conf_fd);

    result<> manage(pid_t pid,
		    const std::string& prog);

    result<> kill(int sig);
    result<> wait();

    std::pair<state_t, int> state() {
	return std::make_pair(state_, value_);
    }

    uint64_t id() {
	return id_;
    }

    pid_t pid() {
	return pid_;
    }

    std::string to_string(const std::string& format_str = "") {
	auto state_str = state_string(state_);
	return c7::format("proc#%{}<%{},pid:%{},%{},%{}>",
			  id_, prog_, pid_, state_str, value_);
    }

    void print(std::ostream& out, const std::string& format_str) {
	auto state_str = state_string(state_);
	return c7::format(out, "proc#%{}<%{},pid:%{},%{},%{}>",
			  id_, prog_, pid_, state_str, value_);
    }
};


std::unordered_set<std::shared_ptr<proc::impl>> proc::impl::procs_;
c7::thread::mutex proc::impl::procs_mutex_;
std::atomic<uint64_t> proc::impl::id_counter_;


// proc::impl -- SIGCHLD handling
// -------------------------------

bool proc::impl::try_wait_raw(bool from_waitprocs)
{
    int status;
    if (::waitpid(pid_, &status, WNOHANG) == pid_) {
	if (WIFEXITED(status)) {
	    state_ = EXIT;
	    value_ = WEXITSTATUS(status);
	} else {
	    state_ = KILLED;
	    value_ = WTERMSIG(status);
	}
	auto self = shared_from_this();
	if (!from_waitprocs) {
	    procs_.erase(self);
	}
	on_finish(proc::proxy(self));
	return true;
    }
    return false;
}

bool proc::impl::try_wait(bool from_waitprocs)
{
    auto defer = cv_.lock();
    if (pid_ != -1) {
	if (try_wait_raw(from_waitprocs)) {
	    cv_.notify_all();
	    return true;
	}
    }
    return false;
}

void proc::impl::waitprocs(int)		// SIGCHLD handler
{
    auto defer = procs_mutex_.lock();

    // range-for DON'T work well with erase
    for (auto cur = procs_.begin(); cur != procs_.end();) {
	auto p = *cur;
	if (p->try_wait(true)) {
	    cur = procs_.erase(cur);
	} else {
	    ++cur;
	}
    }
}


// proc::impl --  fork & execve
// -------------------------------

static result<> fork_x(pid_t& newpid, int& chkpipe)
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
    if (fd_io == target_fd) {
	return true;
    }
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
    pid_t newpid = -1;
    int chkpipe = -1;
    char errval;

    if (auto res = fork_x(newpid, chkpipe); !res) {
	return res.as_error();
    }

    if (newpid == 0) {
	//
	// child process
	//
	if (fd_renumber(chkpipe, conf_fd) && fd_setcloexec(chkpipe)) {
	    if (!preexec || preexec()) {
		// DON'T INHERIT signal mask
		::sigset_t sigs;
		::sigemptyset(&sigs);
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
	if (WIFEXITED(status)) {
	    return c7result_err("program maybe exit soon: exit:%{}", WEXITSTATUS(status));
	} else {
	    return c7result_err("preexec() maybe crashed: signal:%{}", WTERMSIG(status));
	}
    }

    return c7result_ok(newpid);
}


defer
proc::impl::guard_finish()
{
    unguard_ = [](){};	// dummy
    return c7::defer([this](){ unguard_(); });
}


result<>
proc::impl::start(const std::string& prog,
		  const c7::strvec& argv,
		  std::function<bool()> preexec,
		  int conf_fd)
{
    if (state_ != IDLE) {
	return c7result_err("proc object is in use: %{}", to_string());
    }

    prog_ = prog;
    argv_ = argv;

    auto res = forkexec(conf_fd, preexec, prog_, argv_);
    if (!res) {
	state_ = FAILED;
	return c7result_err(std::move(res), "proc::start failed: %{}", to_string());
    }

    // Next lock is important to prevent SIGCHLD handler from accessing this proc
    // object and calling on_finish delegate before on_start.
    auto unlock_defer = cv_.lock();

    auto self = shared_from_this();
    pid_ = res.value();
    state_ = RUNNING;
    {
	auto mutex_defer = procs_mutex_.lock();
 	procs_.insert(self);
    }

    on_start(proc::proxy(self));

    // If forked process exit immediately and SIGCHLD handler is called before
    // process informaton is registered to proc::impl::procs at above code block,
    // the handler cannot find this process and cannot update state of this process.
    // So, state of this process is kept at RUNNING forever without calling
    // try_wait_raw() as follow.
    unlock_defer += [this](){ (void)try_wait_raw(false); };

    if (unguard_) {
	// When callers call proc::guard_finish() before proc::start(), calling
	// unlock_defer() is delayed until guard_finish defer is called.
	unguard_ = std::move(unlock_defer);
    }

    return c7result_ok();
}


result<>
proc::impl::manage(pid_t pid, const std::string& prog)
{
    prog_ = prog;
    argv_ = c7::strvec{prog_};

    // IMPORTANT: same above reason
    auto unlock_defer = cv_.lock();

    pid_ = pid;
    state_ = RUNNING;
    {
	auto mutex_defer = procs_mutex_.lock();
	procs_.insert(shared_from_this());
    }

    // IMPORTANT: same above reason
    unlock_defer += [this](){ (void)try_wait_raw(false); };
    if (unguard_) {
	unguard_ = std::move(unlock_defer);
    }

    return c7result_ok();
}


// proc::impl -- other operations
// -------------------------------

result<> proc::impl::kill(int sig)
{
    auto defer = cv_.lock();
    if (state_ == EXIT || state_ == KILLED) {
	return c7result_ok();
    }
    if (pid_ != -1) {
	if (::kill(pid_, sig) == C7_SYSOK) {
	    return c7result_ok();
	}
    }  else {
	errno = 0;
    }
    return c7result_err(errno, "kill failed: %{}", to_string());
}

result<> proc::impl::wait()
{
    auto defer = cv_.lock();
    while (state_ == RUNNING) {
	cv_.wait();
    }
    if (state_ == EXIT || state_ == KILLED) {
	return c7result_ok();
    }
    return c7result_err("process has not been run: %{}", to_string());
}


/*----------------------------------------------------------------------------
                                     proc
----------------------------------------------------------------------------*/

proc::proc():
    pimpl(new proc::impl()),
    on_start(pimpl->on_start), on_finish(pimpl->on_finish) {
}

c7::defer proc::guard_finish()
{
    return pimpl->guard_finish();
}

result<> proc::_start(const std::string& program,
		      const c7::strvec& argv,
		      std::function<bool()> preexec)

{
    return pimpl->start(program, argv, preexec, conf_fd);
}

result<> proc::manage_external(pid_t pid, const std::string& program)
{
    return pimpl->manage(pid, program);
}

result<> proc::signal(int sig)
{
    return pimpl->kill(sig);
}

result<> proc::wait()
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

pid_t proc::pid() const
{
    return pimpl->pid();
}

std::string proc::to_string(const std::string& format_str) const
{
    return pimpl->to_string(format_str);
}

void proc::print(std::ostream& out, const std::string& format_str) const
{
    pimpl->print(out, format_str);
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

pid_t proc::proxy::pid() const
{
    return pimpl->pid();
}

std::string proc::proxy::to_string(const std::string& format_str) const
{
    return pimpl->to_string(format_str);
}

void proc::proxy::print(std::ostream& out, const std::string& format_str) const
{
    pimpl->print(out, format_str);
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7
