/*
 * c7app.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_APP_HPP_LOADED__
#define C7_APP_HPP_LOADED__


#include <string>
#include <c7result.hpp>


namespace c7 {
namespace app {


// self program name
extern const std::string& progname;
void set_progname(const std::string& name);


// echo

enum class continuation {
    keep, exit_success, exit_failure, abort
};

void echo(continuation con, const char *file, int line);
void echo(continuation con, const char *file, int line, const char *s);
void echo(continuation con, const char *file, int line, const std::string& s);
void echo(continuation con, const char *file, int line, const c7::result_base& res);
void echo(continuation con, const char *file, int line, const c7::result_base& res, const char *s);
void echo(continuation con, const char *file, int line, const c7::result_base& res, const std::string& s);

template <typename Arg1, typename... Args>
void echo(continuation con, const char *file, int line,
	  const char *fmt, const Arg1& arg1, const Args&... args) {
    echo(con, file, line, c7::format(fmt, arg1, args...));
}
template <typename Arg1, typename... Args>
void echo(continuation con, const char *file, int line, const c7::result_base& res,
	  const char *fmt, const Arg1& arg1, const Args&... args) {
    echo(con, file, line, res, c7::format(fmt, arg1, args...));
}
template <typename Arg1, typename... Args>
void echo(continuation con, const char *file, int line,
	  const std::string &fmt, const Arg1& arg1, const Args&... args) {
    echo(con, file, line, c7::format(fmt, arg1, args...));
}
template <typename Arg1, typename... Args>
void echo(continuation con, const char *file, int line, const c7::result_base& res,
	  const std::string& fmt, const Arg1& arg1, const Args&... args) {
    echo(con, file, line, res, c7::format(fmt, arg1, args...));
}


} // namespace app
} // namespace c7


#define c7echo(...)	c7::app::echo(c7::app::continuation::keep, __FILE__, __LINE__, __VA_ARGS__)
#define c7exit(...)	c7::app::echo(c7::app::continuation::exit_success, __FILE__, __LINE__, __VA_ARGS__)
#define c7error(...)	c7::app::echo(c7::app::continuation::exit_failure, __FILE__, __LINE__, __VA_ARGS__)
#define c7abort(...)	c7::app::echo(c7::app::continuation::abort, __FILE__, __LINE__, __VA_ARGS__)


#endif // c7app.hpp
