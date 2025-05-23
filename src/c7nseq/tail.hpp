/*
 * c7nseq/tail.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_TAIL_HPP_LOADED_
#define C7_NSEQ_TAIL_HPP_LOADED_


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


class tail_seq_iterend {};


// tail ------------------------------------------------------------

template <typename Iter, typename Iterend>
class tail_seq_iter {
private:
    using val_type = std::remove_reference_t<decltype(*std::declval<Iter>())>;

    size_t n_;
    uint32_t idx_;
    uint32_t lim_;
    std::vector<val_type> rbuf_;

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= val_type;
    using pointer		= value_type*;
    using reference		= value_type&;

    tail_seq_iter() = default;
    tail_seq_iter(const tail_seq_iter&) = default;
    tail_seq_iter(tail_seq_iter&&) = default;
    tail_seq_iter& operator=(const tail_seq_iter&) = default;
    tail_seq_iter& operator=(tail_seq_iter&&) = default;

    tail_seq_iter(Iter it, Iterend itend, size_t n): n_(n), idx_(0), rbuf_(n) {
	for (; it != itend; ++it, ++idx_) {
	    rbuf_[idx_ % n_] = *it;
	}
	if (idx_ <= n_) {
	    lim_ = idx_;
	    idx_ = 0;
	} else {
	    lim_ = idx_;
	    idx_ -= n_;
	}
    }

    bool operator==(const tail_seq_iter& o) const {
	return (idx_ == o.idx_);
    }

    bool operator!=(const tail_seq_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const tail_seq_iterend&) const {
	return (idx_ == lim_);
    }

    bool operator!=(const tail_seq_iterend&) const {
	return (idx_ != lim_);
    }

    auto& operator++() {
	if (idx_ < lim_) {
	    idx_++;
	}
	return *this;
    }

    decltype(auto) operator*() {
	return rbuf_[idx_ % n_];
    }

    decltype(auto) operator*() const {
	return rbuf_[idx_ % n_];
    }
};


template <typename Seq>
class tail_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    size_t n_;

public:
    tail_seq(Seq seq, size_t n):
	seq_(std::forward<Seq>(seq)), n_(n) {
    }

    tail_seq(const tail_seq&) = delete;
    tail_seq& operator=(const tail_seq&) = delete;
    tail_seq(tail_seq&&) = default;
    tail_seq& operator=(tail_seq&&) = delete;

    auto size() const {
	return (seq_.size() < n_) ? seq_.size() : n_;
    }

    auto begin() {
	using std::begin;
	auto it = begin(seq_);
	using it_type = typename std::iterator_traits<decltype(it)>::iterator_category;
	if constexpr (std::is_same_v<it_type, std::random_access_iterator_tag>) {
	    auto off = (seq_.size() > n_) ? seq_.size() - n_ : 0;
	    return it+off;
	} else {
	    using std::end;
	    auto itend = end(seq_);
	    return tail_seq_iter<decltype(it), decltype(itend)>(it, itend, n_);
	}
    }

    auto end() {
	using std::begin;
	using it_type = typename std::iterator_traits<decltype(begin(seq_))>::iterator_category;
	if constexpr (std::is_same_v<it_type, std::random_access_iterator_tag>) {
	    return begin(seq_) + seq_.size();
	} else {
	    return tail_seq_iterend();
	}
    }

    auto begin() const {
	return const_cast<tail_seq<Seq>*>(this)->begin();
    }

    auto end() const {
	return const_cast<tail_seq<Seq>*>(this)->end();
    }

    auto rbegin() {
	using std::rbegin;
	return rbegin(seq_);
    }

    auto rend() {
	using std::rbegin;
	return rbegin(seq_) + std::min(seq_.size(), n_);
    }

    auto rbegin() const {
	return const_cast<tail_seq<Seq>*>(this)->rbegin();
    }

    auto rend() const {
	return const_cast<tail_seq<Seq>*>(this)->rend();
    }
};


class tail {
public:
    explicit tail(size_t n): n_(n) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return tail_seq<decltype(seq)>(std::forward<Seq>(seq), n_);
    }

private:
    size_t n_;
};


// dtop tail ------------------------------------------------------------

template <typename Iter, typename Iterend>
class drop_tail_seq_iter {
public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= c7::typefunc::remove_cref_t<decltype(*std::declval<Iter>())>;
    using pointer		= value_type*;
    using reference		= value_type&;

    drop_tail_seq_iter() = default;
    drop_tail_seq_iter(const drop_tail_seq_iter&) = default;
    drop_tail_seq_iter(drop_tail_seq_iter&&) = default;
    drop_tail_seq_iter& operator=(const drop_tail_seq_iter&) = default;
    drop_tail_seq_iter& operator=(drop_tail_seq_iter&&) = default;

    drop_tail_seq_iter(Iter it, Iterend itend, size_t n):
	it_(it), itend_(itend), i_(0) {
	for (size_t i = 0; i < n; i++) {
	    if (it_ == itend_) {
		break;
	    }
	    hold_.push_back(std::move(*it_));
	    ++it_;
	}
    }

    bool operator==(const drop_tail_seq_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const drop_tail_seq_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const tail_seq_iterend&) const {
	return (it_ == itend_);
    }

    bool operator!=(const tail_seq_iterend&) const {
	return (it_ != itend_);
    }

    auto& operator++() {
	if (it_ != itend_) {
	    hold_[i_] = std::move(*it_);
	    ++it_;
	    i_ = (i_ + 1) % hold_.size();
	}
	return *this;
    }

    value_type operator*() {		// DON'T use decltype(auto)
	return hold_[i_];
    }

    value_type operator*() const {	// DON't use decltype(auto)
	return hold_[i_];
    }

private:
    Iter it_;
    Iterend itend_;
    size_t i_;
    std::vector<value_type> hold_;
};


template <typename Seq>
class drop_tail_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    size_t n_;

public:
    drop_tail_seq(Seq seq, size_t n):
	seq_(std::forward<Seq>(seq)), n_(n) {
    }

    drop_tail_seq(const drop_tail_seq&) = delete;
    drop_tail_seq& operator=(const drop_tail_seq&) = delete;
    drop_tail_seq(drop_tail_seq&&) = default;
    drop_tail_seq& operator=(drop_tail_seq&&) = delete;

    auto size() const {
	return (seq_.size() < n_) ? 0 : seq_.size() - n_;
    }

    auto begin() {
	using std::begin;
	auto it = begin(seq_);
	using it_type = typename std::iterator_traits<decltype(it)>::iterator_category;
	if constexpr (std::is_same_v<it_type, std::random_access_iterator_tag>) {
	    return it;
	} else {
	    using std::end;
	    auto itend = end(seq_);
	    return drop_tail_seq_iter<decltype(it), decltype(itend)>(it, itend, n_);
	}
    }

    auto end() {
	using std::begin;
	using it_type = typename std::iterator_traits<decltype(begin(seq_))>::iterator_category;
	if constexpr (std::is_same_v<it_type, std::random_access_iterator_tag>) {
	    auto it = begin(seq_);
	    auto eoff = seq_.size() - (n_ < seq_.size() ? n_ : seq_.size());
	    return it+eoff;
	} else {
	    return tail_seq_iterend();
	}
    }

    auto begin() const {
	return const_cast<drop_tail_seq<Seq>*>(this)->begin();
    }

    auto end() const {
	return const_cast<drop_tail_seq<Seq>*>(this)->end();
    }
};


class drop_tail {
public:
    explicit drop_tail(size_t n): n_(n) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return drop_tail_seq<decltype(seq)>(std::forward<Seq>(seq), n_);
    }

private:
    size_t n_;
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <typename Seq>
struct format_ident<c7::nseq::tail_seq<Seq>> {
    static constexpr const char *name = "tail";
};
template <typename Seq>
struct format_ident<c7::nseq::drop_tail_seq<Seq>> {
    static constexpr const char *name = "drop_tail";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/tail.hpp
