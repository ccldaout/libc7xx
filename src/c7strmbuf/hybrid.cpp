/*
 * c7strmbuf/hybrid.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */


#include <c7strmbuf/hybrid.hpp>


namespace c7::strmbuf {


std::streamsize
hybrid::xsputn(const char_type *s, std::streamsize n)
{
    if (pptr() + n < epptr()) {
	std::memcpy(pptr(), s, n);
	pbump(n);
	return n;
    }
    if (vb_available_) {
	auto offp = pptr() - pbase();
	vb_.reserve(vb_.size() + n);
	char *tp = static_cast<char*>(vb_);
	char *ep = tp + vb_.size();
	char *cp = tp + offp;
	std::memcpy(cp, s, n);
	cp += n;
	setp(tp, ep);
	pbump(cp - tp);
    } else {
	vb_available_ = true;
	auto cn = pptr() - pbase();	// pbase == &fv_[0]
	vb_.reserve(cn + n);
	char *tp = static_cast<char*>(vb_);
	char *ep = tp + vb_.size();
	char *cp = tp;
	std::memcpy(cp, pbase(), cn);	// pbase == &fv_[0]
	cp += cn;
	std::memcpy(cp, s, n);
	cp += n;
	setp(tp, ep);
	pbump(cp - tp);
    }
    return n;
}


auto
hybrid::overflow(int_type c) -> int_type
{
    if (traits_type::eq_int_type(c, traits_type::eof())) {
	return traits_type::not_eof(c);
    } else {
	char cc = c;
	xsputn(&cc, 1);
	return c;
    }
}


} // namespace c7::strmbuf
