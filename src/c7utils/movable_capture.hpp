/*
 * c7utils/movable_capture.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_UTILS_MOVABLE_CAPTURE_HPP_LOADED_
#define C7_UTILS_MOVABLE_CAPTURE_HPP_LOADED_
#include <c7common.hpp>


namespace c7 {


/*----------------------------------------------------------------------------
       movable and non-copyable object warapper (for functional object)
----------------------------------------------------------------------------*/

template <typename T>
class movable_capture: private T {
public:
    movable_capture(T& obj): T(std::move(obj)) {}
    movable_capture(const movable_capture& o):
	T(std::move(static_cast<T&&>(const_cast<movable_capture&>(o)))) {
    }
    movable_capture& operator=(const movable_capture& o) {
	return T::operator=(static_cast<T&&>(const_cast<movable_capture&>(o)));
    }
    movable_capture(movable_capture&& o):
	T(std::move(static_cast<T&&>(o))) {
    }
    movable_capture& operator=(movable_capture&& o) {
	return T::operator=(static_cast<T&&>(o));
    }
    T&& unwrap() { return static_cast<T&&>(*this); }
    T&& unwrap() const { return const_cast<T&&>(static_cast<const T&&>(*this)); }
};


} // namespace c7


#endif // c7utils/movable_capture.hpp
