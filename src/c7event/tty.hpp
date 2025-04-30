/*
 * c7event/tty.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_TTY_HPP_LOADED_
#define C7_EVENT_TTY_HPP_LOADED_
#include <c7common.hpp>


#include <sys/ioctl.h>		// struct winsize
#include <sys/termios.h>
#include <vector>
#include <c7fd.hpp>
#include <c7event/monitor.hpp>
#include <c7slice.hpp>


namespace c7::event {


class tty_provider: public c7::event::provider_interface {
public:
    struct tty_interface {
	virtual void tty_input(c7::slice<char> slice) = 0;
	virtual void tty_error(const c7::result_err& err) = 0;
    };

    static void restore_termios();

    explicit tty_provider(tty_interface& svc, bool oflow = false);

    ~tty_provider() {
	restore_termios();
    }

    void on_manage(monitor& m, int) override {
	monitor_ = &m;
    }

    uint32_t default_epoll_events() override;

    int fd() override {
	return tty_;
    }

    void on_event(monitor&, int, uint32_t events) override;

    c7::result<> write(c7::slice<char> slice);

    c7::result<> get_termios(termios& buf);
    c7::result<> raw_termios();
    c7::result<> set_termios(const termios& buf, int action = TCSANOW);
    c7::result<> get_winsize(winsize& buf);

private:
    static termios saved_termios_;
    monitor *monitor_;
    tty_interface& svc_;
    c7::fd tty_;
    std::vector<char> buf_;
    bool oflow_;
};


} // namespace c7::event


#endif // c7event/tty.hpp
