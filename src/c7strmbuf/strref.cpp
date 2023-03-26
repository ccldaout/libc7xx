/*
 * c7strmbuf/strref.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */


#include <c7strmbuf/strref.hpp>


namespace c7::strmbuf {


std::streamsize
strref::xsputn(const char_type *s, std::streamsize n)
{
    vb_.append(s, n);
    setp(vb_.data() + vb_.size(), vb_.data() + vb_.size());
    return n;
}


auto
strref::overflow(int_type c) -> int_type
{
    if (traits_type::eq_int_type(c, traits_type::eof())) {
	return traits_type::not_eof(c);
    } else {
	char cc[2] = {static_cast<char>(c), 0};
	xsputn(cc, 1);
	return c;
    }
}


} // namespace c7::strmbuf
