/*
 * c7nseq/tuple.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_TUPLE_HPP_LOADED__
#define C7_NSEQ_TUPLE_HPP_LOADED__


#include <c7typefunc.hpp>


namespace c7::nseq {


// for_earch for tuple

template <int C, int LIM, typename Tuple, typename F>
inline void tuple_for_each_impl(Tuple&& items, F f)
{
    f(std::get<C>(items));
    if constexpr (C < LIM) {
	tuple_for_each_impl<C+1, LIM>(items, f);
    }
}

template <typename Tuple, typename F>
void tuple_for_each(Tuple&& items, F f)
{
    constexpr int n = c7::typefunc::count_of_tuple_v<Tuple>;
    tuple_for_each_impl<0, n-1>(std::forward<Tuple>(items), f);
}


// transform for tuple

template <int C, int LIM, typename Tuple, typename F, typename... Converted>
inline auto tuple_transform_impl(Tuple&& items, F f, Converted&&... converted)
{
    if constexpr (C == LIM) {
	auto last = f(std::get<C>(items));
	return std::tuple<Converted..., decltype(last)>(std::forward<Converted>(converted)...,
							last);
    } else {
	return tuple_transform_impl<C+1, LIM>(std::forward<Tuple>(items),
					      f,
					      std::forward<Converted>(converted)...,
					      f(std::get<C>(items)));
    }
}

template <typename Tuple, typename F>
auto tuple_transform(Tuple&& items, F f)
{
    constexpr int n = c7::typefunc::count_of_tuple_v<Tuple>;
    return tuple_transform_impl<0, n-1>(std::forward<Tuple>(items), f);
}


} // namespace c7::nseq


#endif // c7nseq/tuple.hpp
