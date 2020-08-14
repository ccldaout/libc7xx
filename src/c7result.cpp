/*
 * c7result.cpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include "c7result.hpp"
#include <cstring>
#include <iomanip>


namespace c7 {


static const int file_w = 16;
static const int line_w = 3;
    

static bool translate_errno(std::ostream& o, int err)
{
    char buff[128];
    const char *p = strerror_r(err, buff, sizeof(buff));
    if (p == nullptr ||	std::strncmp(p, "Unknown", 7) == 0) {
	return false;
    }
    o << "errno:" << err << " " << p;
    return true;
}


inline static void putprefix(std::ostream& out, const char *fn, int line)
{
    out << "[" << std::setw(file_w) << std::right << fn << ":"
	<< std::setw(line_w) << std::dec << line << "] ";
}


void format_traits<result_base>::convert(std::ostream& out, const std::string& format, const result_base& arg)
{
    if (arg) {
	out << "success";
	return;
    }

    auto infos = arg.error();

    if (infos.size() == 0) {
	out << "error: <no information>";
	return;
    }
    out << infos.size() << " error information(s):";

    for (const auto& e: infos) {
	out << "\n";
	const char *fn = e.file;
	if (std::strlen(fn) > file_w)
	    fn = std::strchr(fn, 0) - file_w;

	putprefix(out, fn, e.line);

	if (e.what != 0) {
	    if (translate_what(out, e.what)) {
		if (e.msg.empty()) {
		    continue;
		}
		out << "\n";
		putprefix(out, fn, e.line);
	    } else {
		out << "what:" << std::showbase << std::dec << e.what;
		if (e.msg.empty()) {
		    continue;
		}
		out << ": ";
	    }
	    out << e.msg;
	} else if (e.msg.empty()) {
	    out << "(pass through ...)";
	} else {
	    out << e.msg;
	}
    }
};


c7::delegate<bool, std::ostream&, int>
format_traits<result_base>::translate_what(translate_errno);


} // namespace c7
