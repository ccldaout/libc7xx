/*
 * c7app.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
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

static program_attr program_attr_;
const std::string& progname = program_attr_.name;

void set_progname(const std::string& name)
{
    program_attr_.name = name;
}


static inline void
echo_impl(const char *file, int line, const c7::result_base *res, const char *s)
{
    c7::p_("%{}:%{} %{}", c7path_name(file), line, (s != nullptr) ? s : "...");
    if (res != nullptr) {
	c7::p_("%{}", *res);
    }
}

void echo(const char *file, int line)
{
    echo_impl(file, line, nullptr, nullptr);
}

void echo(const char *file, int line, const char *s)
{
    echo_impl(file, line, nullptr, s);
}

void echo(const char *file, int line, const std::string& s)
{
    echo_impl(file, line, nullptr, s.c_str());
}

void echo(const char *file, int line, const c7::result_base& res)
{
    echo_impl(file, line, &res, nullptr);
}

void echo(const char *file, int line, const c7::result_base& res, const char *s)
{
    echo_impl(file, line, &res, s);
}

void echo(const char *file, int line, const c7::result_base& res, const std::string& s)
{
    echo_impl(file, line, &res, s.c_str());
}


[[noreturn]] static void
quit_impl(quit_type qt, const char *file, int line, const c7::result_base *res, const char *s)
{
    c7::p_("%{}:%{} %{}", c7path_name(file), line, (s != nullptr) ? s : "...");
    if (res != nullptr) {
	c7::p_("%{}", *res);
    }
    switch (qt) {
    case quit_type::success:
	std::quick_exit(EXIT_SUCCESS);
	break;
    case quit_type::failure:
	std::quick_exit(EXIT_FAILURE);
	break;
    case quit_type::abort:
    default:
	std::abort();
	break;
    }
}

[[noreturn]] void
quit(quit_type qt, const char *file, int line)
{
    quit_impl(qt, file, line, nullptr, nullptr);
}

[[noreturn]] void
quit(quit_type qt, const char *file, int line, const char *s)
{
    quit_impl(qt, file, line, nullptr, s);
}

[[noreturn]] void
quit(quit_type qt, const char *file, int line, const std::string& s)
{
    quit_impl(qt, file, line, nullptr, s.c_str());
}

[[noreturn]] void
quit(quit_type qt, const char *file, int line, const c7::result_base& res)
{
    quit_impl(qt, file, line, &res, nullptr);
}

[[noreturn]] void
quit(quit_type qt, const char *file, int line, const c7::result_base& res, const char *s)
{
    quit_impl(qt, file, line, &res, s);
}

[[noreturn]] void
quit(quit_type qt, const char *file, int line, const c7::result_base& res, const std::string& s)
{
    quit_impl(qt, file, line, &res, s.c_str());
}


} // namespace c7::app
