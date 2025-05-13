/*
 * c7string/utf8.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_STRING_UTF8_HPP_LOADED_
#define C7_STRING_UTF8_HPP_LOADED_
#include <c7common.hpp>


#include <string>


namespace c7::utf8 {


bool append_from_utf32(uint32_t u, std::string& s);
std::string from_utf32(uint32_t u, const std::string& alt);
std::string from_utf32(uint32_t u);


} // namespace c7::utf8


#endif // c7string/utf8.hpp
