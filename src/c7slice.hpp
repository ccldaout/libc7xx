/*
 * c7slice.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_SLICE_HPP_LOADED__
#define C7_SLICE_HPP_LOADED__
#include <c7common.hpp>


#include <array>
#include <vector>


namespace c7 {


namespace slice_impl {


template <typename U>
class iterator_impl {
public:
    using difference_type = ptrdiff_t;
    using value_type	= U;
    using pointer	= U*;
    using reference	= U&;
    using iterator_category = std::random_access_iterator_tag;

    iterator_impl(U *p, ptrdiff_t gap): p_(p), gap_(gap) {}
    iterator_impl(const iterator_impl& o): p_(o.p_), gap_(o.gap_) {}
    iterator_impl& operator=(const iterator_impl& o) {
	if (this != &o) {
	    p_ = o.p_;
	    gap_ = o.gap_;
	}
	return *this;
    }

    reference operator[](ptrdiff_t n) const {
	return *(p_ + n * gap_);
    }

    reference operator*() const {
	return *p_;
    }

    pointer operator->() const {
	return p_;
    }

    iterator_impl& operator++() {
	p_ += gap_;
	return *this;
    }

    iterator_impl& operator--() {
	p_ -= gap_;
	return *this;
    }

    iterator_impl operator++(int) {	// postfix
	auto r = *this;
	++(*this);
	return r;
    }

    iterator_impl operator--(int) {	// postfix
	auto r = *this;
	--(*this);
	return r;
    }

    iterator_impl& operator+=(ptrdiff_t n) {
	p_ += (gap_ * n);
	return *this;
    }

    iterator_impl& operator-=(ptrdiff_t n) {
	p_ -= (gap_ * n);
	return *this;
    }

    iterator_impl operator+(ptrdiff_t n) const {
	return iterator_impl(p_ + (gap_ * n), gap_);
    }

    iterator_impl operator-(ptrdiff_t n) const {
	return iterator_impl(p_ - (gap_ * n), gap_);
    }

    difference_type operator-(const iterator_impl& o) const {
	return (p_ - o.p_) / gap_;
    }

    bool operator==(const iterator_impl& o) const {
	return (p_ == o.p_);
    }

    bool operator!=(const iterator_impl& o) const {
	return (p_ != o.p_);
    }

    bool operator<(const iterator_impl& o) const {
	return (p_ < o.p_);
    }

    bool operator<=(const iterator_impl& o) const {
	return (p_ <= o.p_);
    }

    bool operator>(const iterator_impl& o) const {
	return (p_ > o.p_);
    }

    bool operator>=(const iterator_impl& o) const {
	return (p_ >= o.p_);
    }

private:
    U *p_;
    const ptrdiff_t gap_;
};


template <typename T>
static inline void
calculate_range(T * const a_top, const ssize_t a_size,
		ptrdiff_t off, size_t n,
		ptrdiff_t &gap, T *&v_top, T *&v_lim)
{
    if (gap == 0) {
	gap = 1;
    }
    if (gap > 0) {
	v_lim = a_top + a_size;
	if (off >= 0) {
	    off = std::min(off,  a_size);
	    v_top = a_top + off;
	} else {
	    off = std::max(off, -a_size);
	    v_top = a_top + a_size + off;
	}
	n = std::min(static_cast<size_t>((v_lim - v_top + gap - 1) / gap), n);
    } else {
	v_lim = a_top - 1;
	if (off >= 0) {
	    off = std::min(off,  a_size-1);
	    v_top = a_top + off;
	} else {
	    off = std::max(off, -a_size-1);
	    v_top = a_top + a_size + off;
	}
	n = std::min(static_cast<size_t>((v_top - v_lim + -gap - 1) / -gap), n);
    }
    v_lim = v_top + gap * n;
}


} // slice_impl


template <typename T>
class slice {
public:
    using iterator	 = slice_impl::iterator_impl<T>;
    using const_iterator = slice_impl::iterator_impl<const T>;

    slice() {}

    slice(T *top, T *lim, ptrdiff_t gap):
	top_(top), lim_(lim), gap_(gap) {}

    slice(const slice& o):
	top_(o.top_), lim_(o.lim_), gap_(o.gap_) {}

    slice& operator=(const slice& o) {
	top_ = o.top_;
	lim_ = o.lim_;
	gap_ = o.gap_;
	return *this;
    }

    T& operator[](ptrdiff_t n) {
	return *(top_ + gap_ * n);
    }

    const T& operator[](ptrdiff_t n) const {
	return *(top_ + gap_ * n);
    }

    size_t size() const {
	return (lim_ - top_) / gap_;
    }

    T* data() {
	return &(*this)[0];
    }

    auto gap() {
	return gap_;
    }

    const T* data() const {
	return &(*this)[0];
    }

    auto begin() {
	return iterator(top_, gap_);
    }

    auto end() {
	return iterator(lim_, gap_);
    }

    auto begin() const {
	return const_iterator(top_, gap_);
    }

    auto end() const {
	return const_iterator(lim_, 0);
    }

    template <typename C>
    C convert() {
	return C{begin(), end()};
    }

    template <typename C>
    C convert() const {
	return const_cast<slice*>(this)->convert<C>();
    }

    template <template <typename...> class C>
    C<T> convert() {
	return C<T>{begin(), end()};
    }

    template <template <typename...> class C>
    C<T> convert() const {
	return const_cast<slice*>(this)->convert<C>();
    }

    template <typename C>
    std::back_insert_iterator<C> back_insert(C& c) {
	return std::copy(begin(), end(), std::back_inserter(c));
    }

    template <typename C>
    std::back_insert_iterator<C> back_insert(C& c) const {
	return const_cast<slice*>(this)->back_insert(c);
    }

    template <typename C>
    std::back_insert_iterator<C> copy(std::back_insert_iterator<C> it) {
	return std::copy(begin(), end(), it);
    }

    template <typename C>
    std::back_insert_iterator<C> copy(std::back_insert_iterator<C> it) const {
	return const_cast<slice*>(this)->copy(it);
    }

private:
    T* top_ = nullptr;
    T* lim_ = nullptr;
    ptrdiff_t gap_ = 1;
};


template <typename T>
auto make_slice(T& v,
		ptrdiff_t off=0, ptrdiff_t gap=1, size_t n=-1UL)
{
    std::remove_reference_t<decltype(*v.begin())> *v_top;
    std::remove_reference_t<decltype(*v.begin())> *v_lim;
    slice_impl::calculate_range(v.data(), v.size(), off, n, gap, v_top, v_lim);
    return slice(v_top, v_lim, gap);
}


template <typename T, size_t N>
auto make_slice(T (&v)[N],
		ptrdiff_t off=0, ptrdiff_t gap=1, size_t n=-1UL)
{
    T *v_top;
    T *v_lim;
    slice_impl::calculate_range(v, N, off, n, gap, v_top, v_lim);
    return slice(v_top, v_lim, gap);
}


template <typename T>
auto make_slice(T *v, size_t vn,
		ptrdiff_t off=0, ptrdiff_t gap=1, size_t n=-1UL)
{
    T *v_top;
    T *v_lim;
    slice_impl::calculate_range(v, vn, off, n, gap, v_top, v_lim);
    return slice(v_top, v_lim, gap);
}


} // namespace c7


#endif // c7slice.hpp
