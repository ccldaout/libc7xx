/*
 * c7string/eval.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=336895331
 */
#ifndef C7_STRING_EVAL_HPP_LOADED_
#define C7_STRING_EVAL_HPP_LOADED_
#include <c7common.hpp>


#include <ostream>
#include <string>
#include <c7result.hpp>


namespace c7::str {


// eval_C backslash sequece: convert C backslash sequence to a byte

std::string& eval_C(std::string& out, const std::string& s);
std::ostream& eval_C(std::ostream& out, const std::string& s);
std::string eval_C(const std::string& s);


// escape_C: convert a byte to C backslash sequence

std::string& escape_C(std::string& out, const std::string& s, char quote='"');
std::ostream& escape_C(std::ostream& out, const std::string& s, char quote='"');
std::string escape_C(const std::string& s, char quote='"');


// eval: evaluate custom variable reference
//
//	mark:% -> %V %{VAR...}
//      mark:$ -> $V ${VAR...}

typedef std::function<c7::result<const char*>(std::string& out, const char *vn, bool enclosed)> evalvar;

c7::result<> eval(std::string& out, const std::string& in, char mark, char escape, c7::str::evalvar);
c7::result<> eval(std::ostream& out, const std::string& in, char mark, char escape, c7::str::evalvar);
c7::result<std::string> eval(const std::string& in, char mark, char escape, c7::str::evalvar evalvar);


// eval_env: evaluate shell-syntax environment variable reference

c7::result<> eval_env(std::string& out, const std::string& in);
c7::result<> eval_env(std::ostream& out, const std::string& in);
c7::result<std::string> eval_env(const std::string& in);


} // namespace c7::str


#endif // c7string/eval.hpp
