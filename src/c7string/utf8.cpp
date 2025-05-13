/*
 * c7string/utf8.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7format.hpp>
#include <c7string/utf8.hpp>


namespace c7::utf8 {


bool append_from_utf32(uint32_t u, std::string& s)
{
    if ((0xffffff80 & u) == 0) {
	s.push_back(u);
    } else if ((0xfffff800 & u) == 0) {
	s.push_back(0b110'00000 | ((0b11111'000000 & u) >> 6));
	s.push_back(0b10'000000 | ((0b00000'111111 & u)     ));
    } else if ((0xffff0000 & u) == 0) {
	s.push_back(0b1110'0000 | ((0b1111'000000'000000 & u) >> 12));
	s.push_back(0b10'000000 | ((0b0000'111111'000000 & u) >>  6));
	s.push_back(0b10'000000 | ((0b0000'000000'111111 & u)      ));
    } else if ( 0x0010ffff >= u) {
	s.push_back(0b11110'000 | ((0b111'000000'000000'000000 & u) >> 18));
	s.push_back(0b10'000000 | ((0b000'111111'000000'000000 & u) >> 12));
	s.push_back(0b10'000000 | ((0b000'000000'111111'000000 & u) >>  6));
	s.push_back(0b10'000000 | ((0b000'000000'000000'111111 & u)      ));
    } else {
	return false;
    }
    return true;
}


std::string from_utf32(uint32_t u, const std::string& alt)
{
    std::string s;
    if (append_from_utf32(u, s)) {
	return s;
    }
    return alt;
}


std::string from_utf32(uint32_t u)
{
    std::string s;
    if (append_from_utf32(u, s)) {
	return s;
    }
    return c7::format("\\x%{x}", u);
}


} // namespace c7::utf8
