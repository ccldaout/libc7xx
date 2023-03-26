/*
 * c7nseq/_cmn.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ__CMN_HPP_LOADED__
#define C7_NSEQ__CMN_HPP_LOADED__


#include <c7typefunc.hpp>


namespace c7::nseq {


// operator| for sequence

template <typename S1, typename S2>
decltype(auto) operator|(S1&& s1, S2 s2)
{
    if constexpr (c7::typefunc::is_iterable_v<S1>) {
	return s2(std::forward<S1>(s1));
    } else {
	return [s1=std::forward<S1>(s1),
		s2=std::forward<S2>(s2)](auto&& s0) mutable {
	    return s2(s1(std::forward<decltype(s0)>(s0)));
	};
    }
}


} // namespace c7::nseq


#endif // c7nseq/_cmn.hpp
