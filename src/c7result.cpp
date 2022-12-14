/*
 * c7result.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7path.hpp>
#include <c7result.hpp>
#include <cstring>
#include <iomanip>


namespace c7 {


static const int file_w = 16;
static const int line_w = 3;
    

const std::vector<result_base::errinfo> result_base::no_error_;


void result_base::add_errinfo(const char *file, int line, int what)
{
    if (errors_ == nullptr) {
	errors_.reset(new std::vector<errinfo>);
    }
    errors_->push_back(errinfo(file, line, what));
}


void result_base::add_errinfo(const char *file, int line, int what, const char *msg)
{
    if (errors_ == nullptr) {
	errors_.reset(new std::vector<errinfo>);
    }
    errors_->push_back(errinfo(file, line, what, msg));
}


void result_base::add_errinfo(const char *file, int line, int what, std::string&& msg)
{
    if (errors_ == nullptr) {
	errors_.reset(new std::vector<errinfo>);
    }
    errors_->push_back(errinfo(file, line, what, std::move(msg)));
}


void result_base::add_errinfo(const char *file, int line, int what, const std::string& msg)
{
    if (errors_ == nullptr) {
	errors_.reset(new std::vector<errinfo>);
    }
    errors_->push_back(errinfo(file, line, what, msg));
}


void result_base::copy_from(const result_base& src)
{
    if (this != &src) {
	if (src.errors_ == nullptr) {
	    errors_.reset();
	} else {
	    if (errors_ == nullptr) {
		errors_.reset(new std::vector<errinfo>);
	    }
	    *errors_ = *src.errors_;
	}
    }
}


void result_base::merge_iferror(result_base&& src)
{
    if (this != &src && src.errors_ != nullptr) {
	if (errors_ == nullptr) {
	    errors_ = std::move(src.errors_);
	} else {
	    for (auto& e: *src.errors_) {
		errors_->push_back(std::move(e));
	    }
	    src.errors_.reset();
	}
    }
}


void result_base::raise_exception() const
{
    auto err = reinterpret_cast<result_err*>(const_cast<result_base*>(this));
    throw result_exception(std::move(*err));
}


// Obsolete function
void result_base::type_mismatch()
{
    c7::p_("value type mismatch: result data is lost.\n"
	   "Please check:\n"
	   "\tresult<TYPE_A> function(...) {\n"
	   "\t    ...\n"
	   "\t    return c7result_ok(VALUE_OF_TYPE_B);\n"
	   "\t}\n");
    std::abort();
}


// Obsolete function
void result_base::has_no_value()
{
    c7::p_("result has no value");
    std::abort();
}


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
    out << "error information(s):";

    for (const auto& e: infos) {
	out << "\n";
	const char *fn = c7path_name(e.file);
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
	    out << "(no additional message)";
	} else {
	    out << e.msg;
	}
    }
};


c7::delegate<bool, std::ostream&, int>
format_traits<result_base>::translate_what(translate_errno);


} // namespace c7
