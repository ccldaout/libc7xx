/*
 * c7app.cpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <cstdlib>
#include <c7app.hpp>
#include <c7path.hpp>


namespace c7::app {


struct program_attr {
    std::string name = "noname";
    program_attr() {
	char buf[1024];
	ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf)-1);
	if (n != C7_SYSERR) {
	    buf[n] = 0;
	    auto np = c7::strrchr_next(buf, '/', buf);
	    char *sp = const_cast<char *>(c7::strchr_x(np, '.', 0));
	    if (sp != np) {
		*sp = 0;
	    }
	    name = np;
	}
    }
};

static program_attr __program_attr;
const std::string& progname = __program_attr.name;

void set_progname(const std::string& name)
{
    __program_attr.name = name;
}


static void echo_impl(continuation con, const char *file, int line,
		      const c7::result_base *res,
		      const char *s)
{
    c7::p_("%{}:%{} %{}", c7path_name(file), line, (s != nullptr) ? s : "...");
    if (res != nullptr) {
	c7::p_("%{}", *res);
    }
    switch (con) {
    case continuation::keep:
	return;
    case continuation::exit_success:
	std::exit(EXIT_SUCCESS);
	return;
    case continuation::exit_failure:
	std::exit(EXIT_FAILURE);
	return;
    case continuation::abort:
	std::abort();
	return;
    }
}

void echo(continuation con, const char *file, int line)
{
    echo_impl(con, file, line, nullptr, nullptr);
}

void echo(continuation con, const char *file, int line, const char *s)
{
    echo_impl(con, file, line, nullptr, s);
}

void echo(continuation con, const char *file, int line, const std::string& s)
{
    echo_impl(con, file, line, nullptr, s.c_str());
}

void echo(continuation con, const char *file, int line, const c7::result_base& res)
{
    echo_impl(con, file, line, &res, nullptr);
}

void echo(continuation con, const char *file, int line, const c7::result_base& res, const char *s)
{
    echo_impl(con, file, line, &res, s);
}

void echo(continuation con, const char *file, int line, const c7::result_base& res, const std::string& s)
{
    echo_impl(con, file, line, &res, s.c_str());
}


} // namespace c7::app
