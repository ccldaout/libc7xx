/*
 * c7path.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __C7_PATH_HPP_LOADED__
#define __C7_PATH_HPP_LOADED__
#include <c7common.hpp>


#include <c7string.hpp>


namespace c7 {
namespace path {


#define c7path_name(p)		c7::strrchr_next(p, '/', p)
#define c7path_suffix(p)	c7::strrchr_x(p, '.', 0)

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


} // namespace path
} // namespace c7


#endif // c7path.hpp
