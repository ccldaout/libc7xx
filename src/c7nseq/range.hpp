/*
 * c7nseq/range.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_RANGE_HPP_LOADED_
#define C7_NSEQ_RANGE_HPP_LOADED_


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


struct range_iter_end {};


// iterator proxy range --------------------------------------------------

template <typename Iter, typename Iterend>
class range_proxy_iter {
private:
    Iter it_;		// identifier 'it_' is used for _iter_ops.hpp
    Iterend itend_;	// identifier 'itend_' is used for _iter_ops.hpp

public:
    // for STL iterator
    using iterator_category	= typename std::iterator_traits<Iter>::iterator_category;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    range_proxy_iter() = default;
    range_proxy_iter(const range_proxy_iter&) = default;
    range_proxy_iter(range_proxy_iter&&) = default;
    range_proxy_iter& operator=(const range_proxy_iter&) = default;
    range_proxy_iter& operator=(range_proxy_iter&&) = default;

    range_proxy_iter(Iter it, Iterend itend): it_(it), itend_(itend) {}

    bool operator==(const range_proxy_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const range_proxy_iter& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	++it_;
	return *this;
    }

    decltype(auto) operator*() {
	return *it_;
    }

    // for random access

    auto& operator--() {
	--it_;
	return *this;
    }

    decltype(auto) operator[](ptrdiff_t n) const {
	return it_[n];
    }

    auto& operator+=(ptrdiff_t n) {
	it_ += n;
	return *this;
    }

    auto operator+(ptrdiff_t n) const {
	return range_proxy_iter(it_+n, itend_);
    }

    ptrdiff_t operator-(const range_proxy_iter& o) const {
	return it_ - o.it_;
    }

    // introducing operators
#define C7_NSEQ_ITER_TYPE_	range_proxy_iter
#define C7_NSEQ_ITEREND_TYPE_	range_iter_end
#include <c7nseq/_iter_ops.hpp>
};


template <typename Iter, typename Iterend>
class range_proxy_seq {
private:
    Iter it_;
    Iterend itend_;

public:
    range_proxy_seq(Iter it, Iterend itend): it_(it), itend_(itend) {}

    auto begin() {
	return range_proxy_iter(it_, itend_);
    }

    auto end() {
	return range_iter_end();
    }

    auto begin() const {
	return const_cast<range_proxy_seq<Iter, Iterend>*>(this)->begin();
    }

    auto end() const {
	return const_cast<range_proxy_seq<Iter, Iterend>*>(this)->end();
    }
};


// arithmetic range --------------------------------------------------

template <typename T>
class range_iter {
private:
    ssize_t it_;		// identifier 'it_' is used for _iter_ops.hpp
    T beg_, step_;

public:
    // for STL iterator
    using iterator_category	= std::random_access_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= T;
    using pointer		= value_type*;
    using reference		= value_type&;

    range_iter() = default;
    range_iter(const range_iter&) = default;
    range_iter(range_iter&&) = default;
    range_iter& operator=(const range_iter&) = default;
    range_iter& operator=(range_iter&&) = default;

    range_iter(size_t i, T beg, T step):
	it_(i), beg_(beg), step_(step) {
    }

    bool operator==(const range_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const range_iter& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	++it_;
	return *this;
    }

    T operator*() {
	return beg_ + step_ * it_;
    }

    // for random access

    auto& operator--() {
	--it_;
	return *this;
    }

    decltype(auto) operator[](ptrdiff_t n) const {
	return beg_ + step_ * (it_ + n);
    }

    auto& operator+=(ptrdiff_t n) {
	it_ += n;
	return *this;
    }

    auto operator+(ptrdiff_t n) const {
	return range_iter(it_+n, beg_, step_);
    }

    ptrdiff_t operator-(const range_iter& o) const {
	return it_ - o.it_;
    }

    // introducing operators
#define C7_NSEQ_ITER_TYPE_	range_iter
#include <c7nseq/_iter_ops.hpp>
};


template <typename T>
class range_seq {
public:
    range_seq(size_t n, T beg, T step):
	n_(static_cast<ssize_t>(n)), beg_(beg), step_(step) {
    }

    size_t size() const {
	return n_;
    }

    auto begin() {
	return range_iter<T>(0, beg_, step_);
    }

    auto end() {
	return range_iter<T>(n_, beg_, step_);
    }

    auto begin() const {
	return const_cast<range_seq<T>*>(this)->begin();
    }

    auto end() const {
	return const_cast<range_seq<T>*>(this)->end();
    }

    auto rbegin() {
	auto beg = beg_ + step_ * (n_ - 1);
	return range_iter<T>(0, beg, -step_);
    }

    auto rend() {
	auto beg = beg_ + step_ * (n_ - 1);
	return range_iter<T>(n_, beg, -step_);
    }

    auto rbegin() const {
	return const_cast<range_seq<T>*>(this)->rbegin();
    }

    auto rend() const {
	return const_cast<range_seq<T>*>(this)->rend();
    }

private:
    ssize_t n_;
    T beg_, step_;
};


// range factory

template <typename T=int>
auto range(size_t n, T beg=0, T step=1)
{
    return range_seq<T>(n, beg, step);
}


template <typename Iter, typename Iterend,
	  typename = typename std::iterator_traits<Iter>::iterator_category>
auto range(Iter beg, Iterend end)
{
    return range_proxy_seq(beg, end);
}


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <typename T>
struct format_ident<c7::nseq::range_seq<T>> {
    static constexpr const char *name = "range";
};
template <typename Iter, typename Iterend>
struct format_ident<c7::nseq::range_proxy_seq<Iter, Iterend>> {
    static constexpr const char *name = "range++";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/range.hpp
