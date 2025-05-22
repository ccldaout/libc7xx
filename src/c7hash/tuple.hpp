/*
 * c7hash/tuple.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_HASH_TUPLE_HPP_LOADED_
#define C7_HASH_TUPLE_HPP_LOADED_


#include <functional>
#include <c7common.hpp>


template <typename... Ts>
struct std::hash<std::tuple<Ts...>> {
    size_t operator()(const std::tuple<Ts...>& k) const {
	return calculate<0>(k, 17);
    }
    template <size_t I>
    static size_t calculate(const std::tuple<Ts...>& k, size_t v) {
	if constexpr (I == sizeof...(Ts)) {
	    return v;
	} else {
	    using type = c7::typefunc::remove_cref_t<decltype(std::get<I>(k))>;
	    return (31 * v) + std::hash<type>()(std::get<I>(k));
	}
    }
};


#endif // c7hash/tuple.hpp
