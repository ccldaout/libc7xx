/*
 * c7proc.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1296242688
 */
#ifndef C7_PROC_HPP_LOADED_
#define C7_PROC_HPP_LOADED_
#include <c7common.hpp>


#include <c7result.hpp>
#include <c7string.hpp>


namespace c7 {


class proc {
private:
    class impl;
    std::shared_ptr<impl> pimpl;

    result<> _start(const std::string& program,
		    const c7::strvec& argv,
		    std::function<bool()> preexec);

public:
    enum state_t {
	IDLE,		// NOT started
	FAILED,		// fork or exec failed
	RUNNING,	// both fork and exec succeed and not finished
	EXIT,		// process exit
	KILLED		// process is killed
    };

    class proxy {
    public:
	explicit proxy(std::shared_ptr<proc::impl>);
	std::pair<state_t, int> state() const;
	uint64_t id() const;
	pid_t pid() const;
	std::string to_string(const std::string& format_str) const;
	void print(std::ostream& out, const std::string& format) const;

    private:
	std::shared_ptr<proc::impl> pimpl;
    };

    // public member data

    c7::delegate<void, proc::proxy>::proxy on_start;
    c7::delegate<void, proc::proxy>::proxy on_finish;
    int conf_fd = 37;			// temporary used on child process

    // public member functions

    proc();

    c7::defer guard_finish();

    template <typename Preexec, typename... Args>
    result<> start(const std::string& program,
		   const c7::strvec& argv,
		   Preexec preexec, Args... args) {
	return _start(program, argv, [&preexec, &args...](){ return preexec(args...); });
    }

    result<> start(const std::string& program,
		   const c7::strvec& argv) {
	return _start(program, argv, nullptr);
    }

    result<> manage_external(pid_t pid, const std::string& prog);

    result<> signal(int sig);
    result<> wait();

    std::pair<state_t, int> state() const;
    uint64_t id() const;
    pid_t pid() const;
    std::string to_string(const std::string& format_str) const;
    void print(std::ostream& out, const std::string& format) const;
};


} // namespace c7


#endif // c7proc.hpp
