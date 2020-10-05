/*
 * c7path.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7path.hpp>
#include <c7utils.hpp>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <cstdlib>
#include <cstring>


namespace c7 {
namespace path {


// path parts

std::string dir(const std::string& path)
{
    auto n = path.rfind('/');
    if (n == std::string::npos) {
	return "";
    }
    return path.substr(0, n);
}

std::string dir_with_sep(const std::string& path)
{
    auto n = path.rfind('/');
    if (n == std::string::npos) {
	return "";
    }
    return path.substr(0, n+1);
}

std::string nondir(const std::string& path)
{
    return std::string(c7path_name(path.c_str()));
}

std::string name(const std::string& path)
{
    auto p = c7path_name(path.c_str());
    auto q = c7path_suffix(p);
    return std::string(p, q - p);
}

std::string suffix(const std::string& path)
{
    auto p = c7path_name(path.c_str());
    auto q = c7path_suffix(p);
    return std::string(q);
}

std::string nonsuffix(const std::string& path)
{
    auto p = c7path_name(path.c_str());
    auto n = c7path_suffix(p) - path.c_str();
    return path.substr(0, n);
}

bool has_dir(const std::string& path)
{
    return (path.find('/') != std::string::npos);
}






// is_xxx

bool is_dir(const std::string& path)
{
    struct stat st;
    return (stat(path.c_str(), &st) == C7_SYSOK && S_ISDIR(st.st_mode));
}    

bool is_writable(const std::string& path)
{
    return (::access(path.c_str(), W_OK) == C7_SYSOK);
}    

bool is_exists(const std::string& path)
{
    return (::access(path.c_str(), F_OK) == C7_SYSOK);
}


// cwd

result<std::string> cwd()
{
    auto buf = c7::make_unique_cptr<char>();
    
    for (int z = 64; ; z += 64) {
	if (auto np = realloc(buf.get(), z); np == nullptr)
	    return c7result_err(errno, "realloc failed");
	else
	    buf.reset(static_cast<char*>(np));
	if (::getcwd(buf.get(), z) != nullptr) {
	    return c7result_ok(std::string(buf.get()));
	}
	if (errno != ERANGE) {
	    return c7result_err(errno, "getcwd failed");
	}
    }
}


// untildize

std::string untildize(const std::string& path)
{
    const char *p = path.c_str();

    if (p[0] != '~')
	return path;

    std::string out;

    if (p[1] == '/' || p[1] == 0) {
	const char *home = std::getenv("HOME");
	if (home != nullptr && home[0] == '/' && is_dir(home)) {
	    out += home;
	    p++;
	} else {
	    auto res = c7::passwd::by_uid(::geteuid());
	    if (res) {
		out += res.value()->pw_dir;
		p++;
	    }
	}
    } else {
	const char *name_end = c7::strchr_x(p, '/', nullptr);
	std::string user(p+1, name_end - p - 1);
	auto res = c7::passwd::by_name(user);
	if (res) {
	    out += res.value()->pw_dir;
	    p = name_end;
	}
    }
    return (out += p);
}


// ortho

std::string ortho(const std::string& path)
{
    std::vector<std::string> dirv;

    c7::str::split_for(path, '/', [&dirv](const std::string& dir) {
	    if (dir.empty())
		return;
	    if (dir == "..") {
		if (!dirv.empty())
		    dirv.pop_back();
	    } else if (dir == ".") {
		;
	    } else
		dirv.push_back(std::move(dir));
	});

    std::string out;
    for (auto d: dirv)
	(out += '/') += d;
    return out;
}


// abs

result<std::string> abs(const std::string& path, const std::string& base)
{
    auto s = c7::path::untildize(path);
    if (s[0] == '/')
	return c7result_ok(c7::path::ortho(s));

    auto b = c7::path::untildize(base);
    if (b[0] == '/')
	return c7result_ok(c7::path::ortho(b + '/' + path));

    return c7result_err("Both path:'%{}' and base:'%{}' cannot be converted to absolute path.",
			path, base);
}


result<std::string> abs(const std::string& path)
{
    auto s = c7::path::untildize(path);
    if (s[0] == '/')
	return c7result_ok(c7::path::ortho(s));

    auto b = c7::path::cwd();
    if (b)
	return c7result_ok(c7::path::ortho(b.value() + '/' + path));

    return b;
}


// search

template <typename SL>
result<std::string> search_impl(const std::string& name,
				SL pathlist,
				const std::string& default_suffix)
{
    std::string n;
    if (name.find('.') == std::string::npos)
	n = name + default_suffix;
    else
	n = name;

    if (n.find('/') != std::string::npos) {
	if (is_exists(n))
	    return c7result_err(errno, "access failed: %{}", n);
	return c7result_ok(std::move(n));
    }

    for (const auto& paths: pathlist) {
	for (auto& dir: c7::str::split_gen(paths, ':')) {
	    auto n2 = dir + '/' + n;
	    if (is_exists(n2))
		return c7result_ok(std::move(n2));
	}
    }

    return c7result_err(ENOENT, "not found: %{}", n);
}

result<std::string> search(const std::string& name,
			   const std::vector<std::string>& pathlist,
			   const std::string& default_suffix)
{
    return search_impl(name, pathlist, default_suffix);
}

result<std::string> search(const std::string& name,
			   const char *pathlist[],	// nullptr terminated
			   const std::string& default_suffix)
{
    return search_impl(name, c7::seq::charp_array(pathlist), default_suffix);
}


// c7 sepcification file

static bool is_valid_dir(const std::string path)
{
    return (is_dir(path) && is_writable(path));
}

std::string init_c7spec(const std::string& name,
			const std::string& suffix,
			const std::string& envname)
{
    auto n = (c7::strmatch_tail(name, suffix) == 0) ? name : name + suffix;

    if (n.find('/') != std::string::npos)
	return n;

    if (auto s = ::getenv(envname.c_str()); s != nullptr && is_valid_dir(s))
	return std::string(s) + '/' + n;

    if (auto s = ::getenv("C7_ROOT_DIR"); s != nullptr && is_valid_dir(s))
	return std::string(s) + '/' + n;

    auto home = std::string(c7::getenv_x("HOME", ""));
    if (auto dir = home + "/.c7"; is_valid_dir(dir))
	return dir + '/' + n;

    return home + "/." + n;
}

std::string find_c7spec(const std::string& name,
			const std::string& suffix,
			const std::string& envname)
{
    auto n = (c7::strmatch_tail(name, suffix) == 0) ? name : name + suffix;

    if (n.find('/') != std::string::npos)
	return n;

    if (auto s = ::getenv(envname.c_str()); s != nullptr) {
	auto n2 = std::string(s) + '/' + n;
	if (is_exists(n2))
	    return n2;
    }

    if (auto s = ::getenv("C7_ROOT_DIR"); s != nullptr) {
	auto n2 = std::string(s) + '/' + n;
	if (is_exists(n2))
	    return n2;
    }

    auto home = std::string(c7::getenv_x("HOME", ""));
    auto n2 = home + "/.c7/" + n;
    if (is_exists(n2))
	return n2;

    return home + "/." + n;
}


} // namespace path
} // namespace c7
