/*
 * c7nseq/slice.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_SLICE_HPP_LOADED__
#define C7_NSEQ_SLICE_HPP_LOADED__


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


template <typename Iter>
class slice_ra_iter {
private:
    Iter it_;
    ssize_t gap_;

public:
    // for STL iterator
    using iterator_category	= typename std::iterator_traits<Iter>::iterator_category;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    slice_ra_iter() = default;
    slice_ra_iter(const slice_ra_iter&) = default;
    slice_ra_iter(slice_ra_iter&&) = default;
    slice_ra_iter& operator=(const slice_ra_iter&) = default;
    slice_ra_iter& operator=(slice_ra_iter&&) = default;

    slice_ra_iter(Iter it, ssize_t gap): it_(it), gap_(gap) {}

    bool operator==(const slice_ra_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const slice_ra_iter& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	it_ += gap_;
	return *this;
    }

    decltype(auto) operator*() {
	return *it_;
    }

    // for random access

    auto& operator--() {
	it_ -= gap_;
	return *this;
    }

    decltype(auto) operator[](ptrdiff_t n) const {
	return it_[n * gap_];
    }

    auto& operator+=(ptrdiff_t n) {
	it_ += (n * gap_);
	return *this;
    }

    auto operator+(ptrdiff_t n) const {
	return slice_ra_iter(it_ + (n * gap_), gap_);
    }

    ptrdiff_t operator-(const slice_ra_iter& o) const {
	return (it_ - o.it_) / gap_;
    }

    // introducing operators
#define C7_NSEQ_ITER_TYPE__	slice_ra_iter
#include <c7nseq/_iter_ops.hpp>
};


class slice_seq_iterend {};

template <typename Iter, typename Iterend>
class slice_seq_iter {
private:
    Iter it_;
    Iterend itend_;
    ssize_t gap_;
    ssize_t max_;

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    slice_seq_iter() = default;
    slice_seq_iter(const slice_seq_iter&) = default;
    slice_seq_iter(slice_seq_iter&&) = default;
    slice_seq_iter& operator=(const slice_seq_iter&) = default;
    slice_seq_iter& operator=(slice_seq_iter&&) = default;

    slice_seq_iter(Iter it, Iterend itend, ssize_t off, ssize_t gap, ssize_t max):
	it_(it), itend_(itend), gap_(gap), max_(max) {
	for (decltype(off) i = 0; i < off; i++) {
	    if (it_ == itend_) {
		max_ = 0;
		break;
	    }
	    ++it_;
	}
    }

    bool operator==(const slice_seq_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const slice_seq_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const slice_seq_iterend&) const {
	return (max_ == 0);
    }

    bool operator!=(const slice_seq_iterend&) const {
	return (max_ != 0);
    }

    auto& operator++() {
	if (max_ > 0) {
	    for (decltype(gap_) i = 0; i < gap_; i++) {
		if (it_ == itend_) {
		    max_ = 1;
		    break;
		}
		++it_;
	    }
	    max_--;
	}
	return *this;
    }

    decltype(auto) operator*() {
	return *it_;
    }
};


template <typename Seq>
class slice_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    ssize_t off_, gap_, n_;

public:
    slice_seq(Seq seq, ssize_t off, ssize_t gap, ssize_t n):
	seq_(std::forward<Seq>(seq)), off_(off), gap_(gap), n_(n) {
    }

    slice_seq(const slice_seq&) = delete;
    slice_seq& operator=(const slice_seq&) = delete;
    slice_seq(slice_seq&&) = default;
    slice_seq& operator=(slice_seq&&) = delete;

    size_t size() const {
	return std::min((static_cast<ssize_t>(seq_.size()) - off_ + gap_ - 1) / gap_,
			n_);
    }

    auto begin() {
	using std::begin;
	auto it = begin(seq_);
	using it_type = typename std::iterator_traits<decltype(it)>::iterator_category;
	if constexpr (std::is_same_v<it_type, std::random_access_iterator_tag>) {
	    return slice_ra_iter<decltype(it)>(it+off_, gap_);
	} else {
	    using std::end;
	    auto itend = end(seq_);
	    // DON'T use size() because it is possible that seq_ object don't support size().
	    return slice_seq_iter<decltype(it), decltype(itend)>(it, itend, off_, gap_, n_);
	}
    }

    auto end() {
	using std::begin;
	using it_type = typename std::iterator_traits<decltype(begin(seq_))>::iterator_category;
	if constexpr (std::is_same_v<it_type, std::random_access_iterator_tag>) {
	    auto it = begin(seq_);
	    auto eoff = off_ + size() * gap_;
	    return slice_ra_iter<decltype(it)>(it+eoff, gap_);
	} else {
	    return slice_seq_iterend();
	}
    }

    auto begin() const {
	return const_cast<slice_seq<Seq>*>(this)->begin();
    }

    auto end() const {
	return const_cast<slice_seq<Seq>*>(this)->end();
    }

    auto rbegin() {
	auto n	     = static_cast<ssize_t>(size());
	auto seq_n   = static_cast<ssize_t>(seq_.size());
	auto last    = off_ + (n - 1) * gap_;
	auto rev_off = seq_n - 1 - last;

	using std::rbegin;
	auto it = rbegin(seq_);
	using it_type = typename std::iterator_traits<decltype(it)>::iterator_category;

	if constexpr (std::is_same_v<it_type, std::random_access_iterator_tag>) {
	    return slice_ra_iter<decltype(it)>(it+rev_off, gap_);
	} else {
	    using std::rend;
	    auto itend = rend(seq_);
	    return slice_seq_iter<decltype(it), decltype(itend)>(it, itend, rev_off, gap_, n);
	}
    }

    auto rend() {
	auto seq_n   = static_cast<ssize_t>(seq_.size());
	auto beg     = off_;

	using std::rbegin;
	auto it = rbegin(seq_);
	using it_type = typename std::iterator_traits<decltype(it)>::iterator_category;

	if constexpr (std::is_same_v<it_type, std::random_access_iterator_tag>) {
	    auto rev_off = seq_n - 1 - beg + gap_;
	    return slice_ra_iter<decltype(it)>(it+rev_off, gap_);
	} else {
	    return slice_seq_iterend();
	}
    }

    auto rbegin() const {
	return const_cast<slice_seq<Seq>*>(this)->rbegin();
    }

    auto rend() const {
	return const_cast<slice_seq<Seq>*>(this)->rend();
    }
};


class slice {
public:
    slice(size_t off, size_t gap): off_(off), gap_(gap), n_(~(1UL<<63)/gap) {}
    slice(size_t off, size_t gap, size_t n): off_(off), gap_(gap), n_(n) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return slice_seq<decltype(seq)>(std::forward<Seq>(seq), off_, gap_, n_);
    }

private:
    size_t off_, gap_, n_;
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename Seq>
struct format_ident<c7::nseq::slice_seq<Seq>> {
    static constexpr const char *name = "slice";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/slice.hpp
