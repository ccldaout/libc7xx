/*
 * c7nseq/head.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_HEAD_HPP_LOADED_
#define C7_NSEQ_HEAD_HPP_LOADED_


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


class head_iterend {};


// head ------------------------------------------------------------

template <typename Iter, typename Iterend>
class head_seq_iter {
private:
    Iter it_;
    Iterend itend_;
    size_t n_;

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    head_seq_iter() = default;
    head_seq_iter(const head_seq_iter&) = default;
    head_seq_iter(head_seq_iter&&) = default;
    head_seq_iter& operator=(const head_seq_iter&) = default;
    head_seq_iter& operator=(head_seq_iter&&) = default;

    head_seq_iter(Iter it, Iterend itend, size_t n):
	it_(it), itend_(itend), n_(it_ == itend_ ? 0 : n) {
    }

    bool operator==(const head_seq_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const head_seq_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const head_iterend&) const {
	return (n_ == 0);
    }

    bool operator!=(const head_iterend&) const {
	return (n_ != 0);
    }

    auto& operator++() {
	if (n_ > 0) {
	    if (it_ != itend_) {
		++it_;
		--n_;
	    } else {
		n_ = 0;
	    }
	}
	return *this;
    }

    decltype(auto) operator*() {
	return *it_;
    }

    decltype(auto) operator*() const {
	return *it_;
    }
};


template <typename Seq>
class head_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    size_t n_;

public:
    head_seq(Seq seq, size_t n):
	seq_(std::forward<Seq>(seq)), n_(n) {
    }

    // enable copy constructor to keep portability for head defined in old c7seq.hpp
    head_seq(const head_seq& o): seq_(o.seq_), n_(o.n_) {}
    //head_seq(const head_seq&) = delete;
    head_seq& operator=(const head_seq&) = delete;
    head_seq(head_seq&&) = default;
    head_seq& operator=(head_seq&&) = delete;

    auto size() const {
	return (seq_.size() < n_) ? seq_.size() : n_;
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
	    return head_seq_iter<decltype(it), decltype(itend)>(it, itend, n_);
	}
    }

    auto end() {
	using std::begin;
	using it_type = typename std::iterator_traits<decltype(begin(seq_))>::iterator_category;
	if constexpr (std::is_same_v<it_type, std::random_access_iterator_tag>) {
	    return begin(seq_) + size();
	} else {
	    return head_iterend();
	}
    }

    auto begin() const {
	return const_cast<head_seq<Seq>*>(this)->begin();
    }

    auto end() const {
	return const_cast<head_seq<Seq>*>(this)->end();
    }

    auto rbegin() {
	auto rev_off = (seq_.size() > n_) ? seq_.size() - n_ : 0;
	using std::rbegin;
	return rbegin(seq_)+rev_off;
    }

    auto rend() {
	using std::rend;
	return rend(seq_);
    }

    auto rbegin() const {
	return const_cast<head_seq<Seq>*>(this)->rbegin();
    }

    auto rend() const {
	return const_cast<head_seq<Seq>*>(this)->rend();
    }
};


class head {
public:
    explicit head(size_t n): n_(n) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return head_seq<decltype(seq)>(std::forward<Seq>(seq), n_);
    }

private:
    size_t n_;
};


// skip head ------------------------------------------------------------

template <typename Iter, typename Iterend>
class skip_head_seq_iter {
private:
    Iter it_;
    Iterend itend_;
    size_t n_;

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    skip_head_seq_iter() = default;
    skip_head_seq_iter(const skip_head_seq_iter&) = default;
    skip_head_seq_iter(skip_head_seq_iter&&) = default;
    skip_head_seq_iter& operator=(const skip_head_seq_iter&) = default;
    skip_head_seq_iter& operator=(skip_head_seq_iter&&) = default;

    skip_head_seq_iter(Iter it, Iterend itend, size_t n):
	it_(it), itend_(itend), n_(it_ == itend_ ? 0 : n) {
	if (n_ > 0) {
	    do {
		if (it_ != itend_) {
		    ++it_;
		    --n_;
		} else {
		    n_ = 0;
		}
	    } while (n_ > 0);
	}
    }

    bool operator==(const skip_head_seq_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const skip_head_seq_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const head_iterend&) const {
	return (it_ == itend_);
    }

    bool operator!=(const head_iterend&) const {
	return (it_ != itend_);
    }

    auto& operator++() {
	if (it_ != itend_) {
	    ++it_;
	}
	return *this;
    }

    decltype(auto) operator*() {
	return *it_;
    }

    decltype(auto) operator*() const {
	return *it_;
    }
};


template <typename Seq>
class skip_head_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    size_t n_;

public:
    skip_head_seq(Seq seq, size_t n):
	seq_(std::forward<Seq>(seq)), n_(n) {
    }

    skip_head_seq(const skip_head_seq&) = delete;
    skip_head_seq& operator=(const skip_head_seq&) = delete;
    skip_head_seq(skip_head_seq&&) = default;
    skip_head_seq& operator=(skip_head_seq&&) = delete;

    auto size() const {
	return (seq_.size() < n_) ? 0 : seq_.size() - n_;
    }

    auto begin() {
	using std::begin;
	auto it = begin(seq_);
	using it_type = typename std::iterator_traits<decltype(it)>::iterator_category;
	if constexpr (std::is_same_v<it_type, std::random_access_iterator_tag>) {
	    return it + std::min(seq_.size(), n_);
	} else {
	    using std::end;
	    auto itend = end(seq_);
	    return skip_head_seq_iter<decltype(it), decltype(itend)>(it, itend, n_);
	}
    }

    auto end() {
	using std::begin;
	using it_type = typename std::iterator_traits<decltype(begin(seq_))>::iterator_category;
	if constexpr (std::is_same_v<it_type, std::random_access_iterator_tag>) {
	    auto eoff = seq_.size();
	    return begin(seq_) + eoff;
	} else {
	    return head_iterend();
	}
    }

    auto begin() const {
	return const_cast<skip_head_seq<Seq>*>(this)->begin();
    }

    auto end() const {
	return const_cast<skip_head_seq<Seq>*>(this)->end();
    }

    auto rbegin() {
	using std::rbegin;
	return rbegin(seq_);
    }

    auto rend() {
	using std::rbegin;
	return rbegin(seq_) + size();
    }

    auto rbegin() const {
	return const_cast<skip_head_seq<Seq>*>(this)->rbegin();
    }

    auto rend() const {
	return const_cast<skip_head_seq<Seq>*>(this)->rend();
    }
};


class skip_head {
public:
    explicit skip_head(size_t n): n_(n) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return skip_head_seq<decltype(seq)>(std::forward<Seq>(seq), n_);
    }

private:
    size_t n_;
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <typename Seq>
struct format_ident<c7::nseq::head_seq<Seq>> {
    static constexpr const char *name = "head";
};
template <typename Seq>
struct format_ident<c7::nseq::skip_head_seq<Seq>> {
    static constexpr const char *name = "skip_head";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/head.hpp
