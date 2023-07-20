/*
 * c7pipeline.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_PIPELINE_HPP_LOADED__
#define C7_PIPELINE_HPP_LOADED__
#include <c7common.hpp>


#include <memory>
#include <c7result.hpp>
#include <c7string.hpp>


namespace c7 {


class pipeline {
public:
    pipeline();
    ~pipeline();

    pipeline(pipeline&&);
    pipeline& operator=(pipeline&&);

    pipeline& env(const std::string& envval);

    pipeline& env(const std::string& var, const std::string& val) {
	env(var + "=" + val);
	return *this;
    }

    pipeline& add(const std::string& workdir,
		  const std::string& prog,
		  const c7::strvec& argv) {
	return add(workdir, prog, argv, {});
    }

    pipeline& add(const std::string& workdir,
		  const std::string& prog,
		  const c7::strvec& argv,
		  const c7::strvec& envs);	// list of "ENVNAME=VALUE"

    /*
     *   fd0: renumber to 0 in head process of pipeline
     *   fd1: renumber to 1 in tail process of pipeline
     *   fd2: renumber to 2 in all  process of pipeline
     *
     *   + if -1 is specified, connect to /dev/null.
     *   + fd0 and fd1 are closed in this function.
     */
    c7::result<> exec(int fd0, int fd1, int fd2=-1);

    // invoke socketpair and return I/O descriptor
    c7::result<int> exec(int fd2=-1);

    // invoke dual pipe
    c7::result<> exec(int (&pfd)[2], int fd2=-1);

    void kill(int sig);

    // int: 0:success, >0:error (exit code), <0:killed (-signum)
    std::vector<int> wait();

private:
    class impl;
    std::unique_ptr<impl> pimpl_;
};


} // namespace c7


#endif // c7pipeline.hpp
