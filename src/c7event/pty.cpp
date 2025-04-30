/*
 * c7event/pty.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <pty.h>
#include <sys/wait.h>
#include <c7event/pty.hpp>


namespace c7::event {


c7::result<>
pty_provider::start_command(const pty_command& cmd,
			    std::function<void()> preexec)
{
    int fd;
    pid_ = forkpty(&fd, nullptr, &cmd.term, &cmd.winz);
    if (pid_ == 0) {
	if (::chdir(cmd.wd) == C7_SYSERR) {
	    svc_.pty_error(c7result_err(errno, "chdir('%{}') failed", cmd.wd));
	    std::quick_exit(EXIT_FAILURE);
	}

	if (setregid(cmd.rgid, cmd.egid) == C7_SYSERR) {
	    svc_.pty_error(c7result_err(errno, "setregid(%{}, %{}) failed", cmd.rgid, cmd.egid));
	    std::quick_exit(EXIT_FAILURE);
	}

	if (setreuid(cmd.ruid, cmd.euid) == C7_SYSERR) {
	    svc_.pty_error(c7result_err(errno, "setreuid(%{}, %{}) failed", cmd.ruid, cmd.euid));
	    std::quick_exit(EXIT_FAILURE);
	}

	if (preexec) {
	    preexec();
	}

	execvpe(cmd.argv[0], cmd.argv, cmd.env);
	svc_.pty_error(c7result_err(errno, "execvpe('%{}') failed", cmd.argv[0]));
	std::quick_exit(EXIT_FAILURE);
    } else if (pid_ == C7_SYSERR) {
	return c7result_err(errno, "forkpty() failed");
    }

    master_ = std::move(c7::fd{fd});
    master_.on_close += [this](auto& fd){
			    if (monitor_) {
				// .... require LOG interface
				monitor_->unmanage(fd);
			    }
			};
    return c7result_ok();
}


c7::result<>
pty_provider::change_winsize(const winsize& winz)
{
    if (::ioctl(master_, TIOCSWINSZ, &winz) == C7_SYSERR) {
	return c7result_err(errno, "ioctl(%{}, TIOCSWINSZ) failed", master_);
    }
    return c7result_ok();
}


void
pty_provider::on_event(monitor&, int, uint32_t)
{
    char buff[128];

    if (auto res = master_.read(buff); !res) {
	svc_.pty_error(c7result_err(errno, "read(master) failed"));
	master_.close();

	int status;
	::waitpid(pid_, &status, 0);
	status = WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
	svc_.pty_closed(status);
	return;
    } else {
	svc_.pty_input(c7::make_slice<char>(buff, res.value()));
    }
}


} // namespace c7::event
