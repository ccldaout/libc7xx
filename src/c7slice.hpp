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
#ifndef C7_SLICE_HPP_LOADED_
#define C7_SLICE_HPP_LOADED_
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

    iterator_impl(U *top, ptrdiff_t idx, ptrdiff_t gap): top_(top), idx_(idx), gap_(gap) {}
    iterator_impl(const iterator_impl& o): top_(o.top_), idx_(o.idx_), gap_(o.gap_) {}
    iterator_impl& operator=(const iterator_impl& o) {
	if (this != &o) {
	    top_ = o.top_;
	    idx_ = o.idx_;
	    gap_ = o.gap_;
	}
	return *this;
    }

    reference operator[](ptrdiff_t n) const {
	return *(top_ + n * gap_);
    }

    reference operator*() const {
	return *(top_ + idx_ * gap_);
    }

    pointer operator->() const {
	return (top_ + idx_ * gap_);
    }

    iterator_impl& operator++() {
	idx_++;
	return *this;
    }

    iterator_impl& operator--() {
	idx_--;
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
	idx_ += n;
	return *this;
    }

    iterator_impl& operator-=(ptrdiff_t n) {
	idx_ -= n;
	return *this;
    }

    iterator_impl operator+(ptrdiff_t n) const {
	return iterator_impl(top_, idx_ + n, gap_);
    }

    iterator_impl operator-(ptrdiff_t n) const {
	return iterator_impl(top_, idx_ - n, gap_);
    }

    difference_type operator-(const iterator_impl& o) const {
	return idx_ - o.idx_;
    }

    bool operator==(const iterator_impl& o) const {
	return (idx_ == o.idx_);
    }

    bool operator!=(const iterator_impl& o) const {
	return !(*this == o);
    }

    bool operator<(const iterator_impl& o) const {
	return (idx_ < o.idx_);
    }

    bool operator<=(const iterator_impl& o) const {
	return (idx_ <= o.idx_);
    }

    bool operator>(const iterator_impl& o) const {
	return !(*this <= o);
    }

    bool operator>=(const iterator_impl& o) const {
	return !(*this < o);
    }

private:
    U *top_;
    ptrdiff_t idx_;
    const ptrdiff_t gap_;
};


template <typename T>
static inline void
calculate_range(T * const a_top, const ssize_t a_n, ptrdiff_t a_off,
		T *&v_top, size_t &v_n, ptrdiff_t &gap)
{
    if (gap == 0) {
	gap = 1;
    }
    if (a_off < 0) {
	a_off = a_n + a_off;
    }
    a_off = std::max(std::min(a_off, a_n), 0L);
    if (gap > 0) {
	v_n = std::min(static_cast<size_t>((a_n - a_off + gap - 1) / gap), v_n);
    } else {
	v_n = std::min(static_cast<size_t>(((a_off+1) + (-gap) - 1) / (-gap)), v_n);
    }
    v_top = a_top + a_off;
}


} // slice_impl


template <typename T>
class slice {
public:
    using iterator	 = slice_impl::iterator_impl<T>;
    using const_iterator = slice_impl::iterator_impl<const T>;

    slice() {}

    slice(T *top, ptrdiff_t n, ptrdiff_t gap):
	top_(top), n_(n), gap_(gap) {}

    slice(const slice& o):
	top_(o.top_), n_(o.n_), gap_(o.gap_) {}

    slice& operator=(const slice& o) {
	top_ = o.top_;
	n_   = o.n_;
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
	return n_;
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
	return iterator(top_, 0, gap_);
    }

    auto end() {
	return iterator(top_, n_, gap_);
    }

    auto begin() const {
	return const_iterator(top_, 0, gap_);
    }

    auto end() const {
	return const_iterator(top_, n_, 0);
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
    ptrdiff_t n_   = 0;
    ptrdiff_t gap_ = 1;
};


template <typename T>
auto make_slice(T& v,
		ptrdiff_t off=0, ptrdiff_t gap=1, size_t n=-1UL)
{
    std::remove_reference_t<decltype(*v.begin())> *v_top;
    slice_impl::calculate_range(v.data(), v.size(), off, v_top, n, gap);
    return slice(v_top, n, gap);
}


template <typename T, size_t N>
auto make_slice(T (&v)[N],
		ptrdiff_t off=0, ptrdiff_t gap=1, size_t n=-1UL)
{
    T *v_top;
    slice_impl::calculate_range(v, N, off, v_top, n, gap);
    return slice(v_top, n, gap);
}


template <typename T>
auto make_slice(T *v, size_t vn,
		ptrdiff_t off=0, ptrdiff_t gap=1, size_t n=-1UL)
{
    T *v_top;
    slice_impl::calculate_range(v, vn, off, v_top, n, gap);
    return slice(v_top, n, gap);
}


template <typename T>
auto make_slice(c7::slice<T>& s,
		ptrdiff_t off=0, ptrdiff_t gap=1, size_t n=-1UL)
{
    ptrdiff_t s_n = s.size();
    ptrdiff_t s_g = s.gap();
    if (off < 0) {
	off = s_n + off;
    }
    off = std::max(std::min(off, s_n), 0L);

    if (gap == 0) {
	gap = 1;
    }

    T *v_top = s.data() + off * s_g;
    if (gap > 0) {
	n = std::min<size_t>(n, (s_n - off + gap - 1)/gap);
    } else {
	n = std::min<size_t>(n, (off + 1 + (-gap) - 1)/(-gap));
    }
    gap *= s_g;
    return slice(v_top, n, gap);
}


} // namespace c7


#endif // c7slice.hpp
