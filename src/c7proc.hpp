/*
 * c7proc.hpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __C7_PROC_HPP_LOADED__
#define __C7_PROC_HPP_LOADED__
#include <c7common.hpp>


#include <c7result.hpp>
#include <c7string.hpp>
#include <c7thread.hpp>


namespace c7 {


class proc {
public:
    enum state_t {
	IDLE,		// NOT started
	FAILED,		// fork or exec failed
	RUNNING,	// both fork and exec succeed and not finished
	EXIT,		// process exit
	KILLED		// process is killed
    };

private:
    class impl;
    std::shared_ptr<impl> pimpl;

public:
    class proxy;

    c7::delegate<void, proc::proxy>::proxy on_start;
    c7::delegate<void, proc::proxy>::proxy on_finish;
    int conf_fd = 37;			// temporary used on child process

    proc();

    template <typename Preexec, typename... Args>
    result<void> start(const std::string& program,
		       const c7::strvec& argv,
		       Preexec preexec, Args... args) {
	return _start(program, argv, [&preexec, &args...](){ return preexec(args...); });
    }

    result<void> start(const std::string& program,
		       const c7::strvec& argv) {
	return _start(program, argv, nullptr);
    }

    void manage_external(pid_t pid, const std::string& prog);

    result<void> signal(int sig);
    result<void> wait();
    std::pair<state_t, int> state() const;
    uint64_t id() const;
    std::string format(const std::string& format_str) const;

public:
    class proxy {
    private:
	std::shared_ptr<proc::impl> pimpl;

    public:
	explicit proxy(std::shared_ptr<proc::impl>);
	std::pair<state_t, int> state() const;
	uint64_t id() const;
	std::string format(const std::string& format_str) const;
    };

    result<void> _start(const std::string& program,
			const c7::strvec& argv,
			std::function<bool()> preexec);
};


template <>
struct format_traits<proc> {
    static void convert(std::ostream& out, const std::string& format, const proc& obj) {
	out << obj.format(format);
    }
};

template <>
struct format_traits<proc::proxy> {
    static void convert(std::ostream& out, const std::string& format, const proc::proxy& obj) {
	out << obj.format(format);
    }
};


} // namespace c7


#endif // c7proc.hpp
