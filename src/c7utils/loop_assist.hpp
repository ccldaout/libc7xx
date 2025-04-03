/*
 * c7utils/loop_assist.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_UTILS_LOOP_ASSIST_HPP_LOADED_
#define C7_UTILS_LOOP_ASSIST_HPP_LOADED_
#include <c7common.hpp>


#include <tuple>


namespace c7 {


/*----------------------------------------------------------------------------
                                loop assistant
----------------------------------------------------------------------------*/

template <typename C>
inline std::tuple<ptrdiff_t,ptrdiff_t,ptrdiff_t> loop_params(const C& c, bool ascendant)
{
    if (ascendant) {
	return std::make_tuple(0, (ptrdiff_t)c.size(), 1);
    } else {
	return std::make_tuple((ptrdiff_t)c.size() - 1, -1, -1);
    }
}


} // namespace c7


#endif // c7utils/loop_assist.hpp
