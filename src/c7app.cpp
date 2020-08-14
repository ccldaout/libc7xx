/*
 * c7app.cpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <unistd.h>
#include <c7string.hpp>
#include "c7app.hpp"


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


} // namespace c7::app
