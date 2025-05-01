/*
 * c7event/tty.cpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <c7event/tty.hpp>


namespace c7::event {


termios tty_provider::saved_termios_;


tty_provider::tty_provider(tty_interface& svc, bool oflow):
    svc_(svc), tty_(STDIN_FILENO), oflow_(oflow)
{
    get_termios(saved_termios_);
    std::at_quick_exit(tty_provider::restore_termios);
}


void
tty_provider::restore_termios()
{
    ::tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios_);
}


uint32_t
tty_provider::default_epoll_events()
{
    return oflow_ ? EPOLLIN|EPOLLOUT : EPOLLIN;
}


void
tty_provider::on_event(monitor&, int, uint32_t events)
{
    if ((events & EPOLLOUT) != 0) {
	if (!buf_.empty()) {
	    if (auto res = tty_.write(buf_.data(), buf_.size()); !res) {
		svc_.tty_error(res.as_error());
	    } else {
		buf_.erase(buf_.begin(), buf_.begin()+res.value());
	    }
	}
	if (buf_.empty()) {
	    if (auto res = monitor_->change_event(tty_, EPOLLIN); !res) {
		svc_.tty_error(res.as_error());
	    }
	}
    }
    if ((events & EPOLLIN) != 0) {
	char buff[128];
	if (auto res = tty_.read(buff); !res) {
	    svc_.tty_error(res.as_error());
	} else {
	    svc_.tty_input(c7::make_slice(buff, res.value()));
	}
    }
}


c7::result<>
tty_provider::write(c7::slice<char> slice)
{
    if (oflow_) {
	slice.copy_to(buf_);
	return monitor_->change_event(tty_, EPOLLIN|EPOLLOUT);
    } else {
	return tty_.write_n(slice.data(), slice.size()).as_error();
    }
}


c7::result<>
tty_provider::get_termios(termios& buf)
{
    if (tcgetattr(tty_, &buf) == C7_SYSERR) {
	return c7result_err(errno, "tcgetattr(%{}) failed", tty_);
    }
    return c7result_ok();
}


c7::result<>
tty_provider::set_termios(const termios& buf, int action)
{
    if (tcsetattr(tty_, action, &buf) == C7_SYSERR) {
	return c7result_err(errno, "tcsetattr(%{}) failed", tty_);
    }
    return c7result_ok();
}


c7::result<>
tty_provider::raw_termios()
{
    termios raw_termios{saved_termios_};
    raw_termios.c_lflag &= ~(ICANON|ISIG|IEXTEN|ECHO);
    raw_termios.c_iflag &= ~(BRKINT|ICRNL|IGNBRK|IGNCR|INLCR|INPCK|ISTRIP|IXON|PARMRK);
    raw_termios.c_oflag &= ~OPOST;
    raw_termios.c_cc[ VMIN] = 1;
    raw_termios.c_cc[VTIME] = 0;
    return set_termios(raw_termios);
}


c7::result<>
tty_provider::get_winsize(winsize& buf)
{
    if (::ioctl(tty_, TIOCGWINSZ, &buf) == C7_SYSERR) {
	return c7result_err(errno, "ioctl(%{}, TIOCGWINSZ) failed", tty_);
    }
    return c7result_ok();
}


} // namespace c7::event
