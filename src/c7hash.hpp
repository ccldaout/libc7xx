/*
 * c7hash.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_HASH_HPP_LOADED__
#define C7_HASH_HPP_LOADED__


#include <functional>
#include <c7common.hpp>


namespace std {


#define C7_HASH_SIMPLE_WRAP__	(1)	// hash for c7::simple_wrapper<,>
template <typename T, typename Tag>
struct hash<c7::simple_wrap<T, Tag>>: public hash<T> {
    size_t operator()(c7::simple_wrap<T, Tag> k) const {
	return hash<T>()(k);
    }
};


} // namespace std


#endif // c7hash.hpp
