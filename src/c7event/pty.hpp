/*
 * c7event/pty.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_PTY_HPP_LOADED_
#define C7_EVENT_PTY_HPP_LOADED_
#include <c7common.hpp>


#include <sys/ioctl.h>		// struct winsize
#include <sys/termios.h>
#include <c7event/monitor.hpp>
#include <c7fd.hpp>
#include <c7slice.hpp>


namespace c7::event {


class pty_provider: public c7::event::provider_interface {
public:
    struct pty_interface {
	virtual void pty_input(c7::slice<char> slice) = 0;
	virtual void pty_closed(int status) = 0;
	virtual void pty_error(const c7::result_err& err) = 0;
    };

    struct pty_command {
	termios term;
	winsize winz;
	uid_t ruid = C7_SYSERR;
	uid_t euid = C7_SYSERR;
	gid_t rgid = C7_SYSERR;
	gid_t egid = C7_SYSERR;
	const char *wd;
	char **argv;
	char **env;
    };

    explicit pty_provider(pty_interface& svc): svc_(svc) {}

    c7::result<> start_command(const pty_command& command,
			       std::function<void()> preexec = nullptr);

    c7::result<> change_winsize(const winsize& winz);

    c7::io_result write(c7::slice<char> slice) {
	return master_.write_n(slice.data(), slice.size());
    }

    int fd() override {
	return master_;
    }

    void on_manage(monitor& m, int prvfd) override {
	monitor_ = &m;
    }

    void on_event(monitor&, int, uint32_t) override;

    void close() {
	master_.close();
    }

private:
    monitor *monitor_;
    pty_interface& svc_;
    c7::fd master_;
    pid_t pid_ = C7_SYSERR;
};


} // namespace c7::event


#endif // c7event/pty.hpp
