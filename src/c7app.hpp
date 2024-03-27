/*
 * c7app.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=0
 */
#ifndef C7_APP_HPP_LOADED_
#define C7_APP_HPP_LOADED_


#include <string>
#include <c7result.hpp>


namespace c7 {
namespace app {


// self program name
extern const std::string& progname;
void set_progname(const std::string& name);


// echo

void echo(const char *file, int line);
void echo(const char *file, int line, const char *s);
void echo(const char *file, int line, const std::string& s);
void echo(const char *file, int line, const c7::result_base& res);
void echo(const char *file, int line, const c7::result_base& res, const char *s);
void echo(const char *file, int line, const c7::result_base& res, const std::string& s);

template <typename Arg1, typename... Args>
void echo(const char *file, int line,
	  const char *fmt, const Arg1& arg1, const Args&... args) {
    echo(file, line, c7::format(fmt, arg1, args...));
}
template <typename Arg1, typename... Args>
void echo(const char *file, int line, const c7::result_base& res,
	  const char *fmt, const Arg1& arg1, const Args&... args) {
    echo(file, line, res, c7::format(fmt, arg1, args...));
}
template <typename Arg1, typename... Args>
void echo(const char *file, int line,
	  const std::string &fmt, const Arg1& arg1, const Args&... args) {
    echo(file, line, c7::format(fmt, arg1, args...));
}
template <typename Arg1, typename... Args>
void echo(const char *file, int line, const c7::result_base& res,
	  const std::string& fmt, const Arg1& arg1, const Args&... args) {
    echo(file, line, res, c7::format(fmt, arg1, args...));
}


// echo + terminate

enum class quit_type {
    success, failure, abort
};

[[noreturn]] void quit(quit_type qt, const char *file, int line);
[[noreturn]] void quit(quit_type qt, const char *file, int line, const char *s);
[[noreturn]] void quit(quit_type qt, const char *file, int line, const std::string& s);
[[noreturn]] void quit(quit_type qt, const char *file, int line, const c7::result_base& res);
[[noreturn]] void quit(quit_type qt, const char *file, int line, const c7::result_base& res, const char *s);
[[noreturn]] void quit(quit_type qt, const char *file, int line, const c7::result_base& res, const std::string& s);

template <typename Arg1, typename... Args>
[[noreturn]] void
quit(quit_type qt, const char *file, int line,
     const char *fmt, const Arg1& arg1, const Args&... args) {
    quit(qt, file, line, c7::format(fmt, arg1, args...));
}
template <typename Arg1, typename... Args>
[[noreturn]] void
quit(quit_type qt, const char *file, int line, const c7::result_base& res,
     const char *fmt, const Arg1& arg1, const Args&... args) {
    quit(qt, file, line, res, c7::format(fmt, arg1, args...));
}
template <typename Arg1, typename... Args>
[[noreturn]] void
quit(quit_type qt, const char *file, int line,
     const std::string &fmt, const Arg1& arg1, const Args&... args) {
    quit(qt, file, line, c7::format(fmt, arg1, args...));
}
template <typename Arg1, typename... Args>
[[noreturn]] void
quit(quit_type qt, const char *file, int line, const c7::result_base& res,
     const std::string& fmt, const Arg1& arg1, const Args&... args) {
    quit(qt, file, line, res, c7::format(fmt, arg1, args...));
}


} // namespace app
} // namespace c7


#define c7echo(...)	c7::app::echo(__FILE__, __LINE__, __VA_ARGS__)
#define c7exit(...)	c7::app::quit(c7::app::quit_type::success, __FILE__, __LINE__, __VA_ARGS__)
#define c7error(...)	c7::app::quit(c7::app::quit_type::failure, __FILE__, __LINE__, __VA_ARGS__)
#define c7abort(...)	c7::app::quit(c7::app::quit_type::abort, __FILE__, __LINE__, __VA_ARGS__)


#endif // c7app.hpp
