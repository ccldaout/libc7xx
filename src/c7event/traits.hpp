/*
 * c7event/traits.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_TRAITS_HPP_LOADED_
#define C7_EVENT_TRAITS_HPP_LOADED_
#include <c7common.hpp>


namespace c7::event {


template <typename T>
struct lock_traits {
    // Requirements for specialized class:
    // - has_lock     : true if and only if T has lock mechanism.
    // - lock()       : do lock and return c7::defer like object if has_lock is true.
    // - lock_ifimpl(): do lock if has_lock and return c7::defer like object always.

    static inline constexpr bool has_lock = false;
    static auto lock(T&);
    static auto lock_ifimpl(T&) {
	struct empty_raii { ~empty_raii() {} };
	return empty_raii{};
    }
};


} // namespace c7::event


#endif // c7event/traits.hpp
