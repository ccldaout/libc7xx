/*
 * c7path.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1462755396
 */
#ifndef C7_PATH_HPP_LOADED_
#define C7_PATH_HPP_LOADED_
#include <c7common.hpp>


#include <c7result.hpp>
#include <c7string/c_str.hpp>


namespace c7::path {


#define c7path_name(p)		c7::strrchr_next(p, '/', p)
#define c7path_suffix(p)	c7::strrchr_x(p, '.', 0)

std::string dir(const std::string& path);		// without last '/'

std::string dir_with_sep(const std::string& path);	// with last '/'

std::string nondir(const std::string& path);		// name + siffix

std::string name(const std::string& path);		// without suffix

std::string suffix(const std::string& path);		// include leading '.'

std::string nonsuffix(const std::string& path);		// dir + name

bool has_dir(const std::string& path);

bool is_dir(const std::string& path);

bool is_writable(const std::string& path);

bool is_exists(const std::string& path);

result<std::string> cwd();

std::string untildize(const std::string& path);

std::string ortho(const std::string& path);

result<std::string> abs(const std::string& path, const std::string& base);

result<std::string> abs(const std::string& path);

result<std::string> search(const std::string& name,
			   const std::vector<std::string>& pathlist,
			   const std::string& default_suffix = "");

result<std::string> search(const std::string& name,
			   const char *pathlist[],	// nullptr terminated
			   const std::string& default_suffix = "");

std::string init_c7spec(const std::string& name,
			const std::string& suffix,
			const std::string& envname = "");

std::string find_c7spec(const std::string& name,
			const std::string& suffix,
			const std::string& envname = "");


} // namespace c7::path


#endif // c7path.hpp
