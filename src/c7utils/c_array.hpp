/*
 * c7utils/c_array.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_UTILS_C_ARRAY_HPP_LOADED_
#define C7_UTILS_C_ARRAY_HPP_LOADED_
#include <c7common.hpp>


#include <iterator>


namespace c7 {


/*----------------------------------------------------------------------------
                            C array type iterator
----------------------------------------------------------------------------*/

template <typename T>
class c_array_iterator {
private:
    T *top_;
    ptrdiff_t idx_;

public:
    using difference_type	= ptrdiff_t;
    using value_type		= T;
    using pointer		= T*;
    using reference		= T&;
    using iterator_category	= std::random_access_iterator_tag;

    c_array_iterator(): top_(nullptr), idx_(0) {}
    c_array_iterator(T *top, size_t idx): top_(top), idx_((ptrdiff_t)idx) {}
    c_array_iterator(const c_array_iterator& o): top_(o.top_), idx_(o.idx_) {}
    c_array_iterator& operator=(const c_array_iterator& o) {
	top_ = o.top_;
	idx_ = o.idx_;
	return *this;
    }

    T& operator[](ptrdiff_t n) const {
	return top_[idx_ + n];
    }

    T& operator*() const {
	return top_[idx_];
    }

    T* operator->() const {
	return &top_[idx_];
    }

    c_array_iterator<T>& operator++() {	// prefix
	idx_++;
	return *this;
    }

    c_array_iterator<T>& operator--() {	// prefix
	idx_--;
	return *this;
    }

    c_array_iterator<T> operator++(int) {	// postfix
	size_t i = idx_++;
	return c_array_iterator<T>(top_, i);
    }

    c_array_iterator<T> operator--(int) {	// postfix
	size_t i = idx_--;
	return c_array_iterator<T>(top_, i);
    }

    c_array_iterator<T>& operator+=(ptrdiff_t n) {
	idx_ += n;
	return *this;
    }

    c_array_iterator<T>& operator-=(ptrdiff_t n) {
	idx_ -= n;
	return *this;
    }

    c_array_iterator<T> operator+(ptrdiff_t n) const {
	return c_array_iterator<T>(top_, idx_ + n);
    }

    c_array_iterator<T> operator-(ptrdiff_t n) const {
	return c_array_iterator<T>(top_, idx_ - n);
    }

    ptrdiff_t operator-(const c_array_iterator<T>& o) const {
	return idx_ - o.idx_;
    }

    bool operator==(const c_array_iterator<T>& o) const {
	return idx_ == o.idx_;
    }

    bool operator!=(const c_array_iterator<T>& o) const {
	return idx_ != o.idx_;
    }

    bool operator<(const c_array_iterator<T>& o) const {
	return idx_ < o.idx_;
    }

    bool operator<=(const c_array_iterator<T>& o) const {
	return idx_ <= o.idx_;
    }

    bool operator>(const c_array_iterator<T>& o) const {
	return idx_ > o.idx_;
    }

    bool operator>=(const c_array_iterator<T>& o) const {
	return idx_ >= o.idx_;
    }
};


} // namespace c7


#endif // c7utils/c_array.hpp
