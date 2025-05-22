/*
 * c7hash/pair.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_HASH_PAIR_HPP_LOADED_
#define C7_HASH_PAIR_HPP_LOADED_


#include <functional>
#include <c7common.hpp>


template <typename P1, typename P2>
struct std::hash<std::pair<P1, P2>> {
    size_t operator()(const std::pair<P1, P2>& k) const {
	size_t v = 17;
	v = (31 * v) + std::hash<P1>()(k.first);
	v = (31 * v) + std::hash<P2>()(k.second);
	return v;
    }
};


#endif // c7hash/pair.hpp
