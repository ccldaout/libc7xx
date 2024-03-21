/*
 * c7nseq/window.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_WINDOW_HPP_LOADED__
#define C7_NSEQ_WINDOW_HPP_LOADED__


#include <c7nseq/_cmn.hpp>
#include <c7nseq/range.hpp>


namespace c7::nseq {


struct window_iter_end {};


// iterator : window as range with iterator pair

template <typename Iter, typename Iterend>
class window_range_iter {
private:
    Iter it_;
    Iterend itend_;
    size_t n_ = 0;

    Iter wb_;

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= decltype(range(wb_, it_));
    using pointer		= value_type*;
    using reference		= value_type&;

    window_range_iter() = default;
    window_range_iter(const window_range_iter&) = default;
    window_range_iter(window_range_iter&&) = default;
    window_range_iter& operator=(const window_range_iter&) = default;
    window_range_iter& operator=(window_range_iter&&) = default;

    window_range_iter(Iter it, Iterend itend, size_t n): it_(it), itend_(itend), n_(n) {
	wb_ = it_;
	for (size_t i = 0; i < n; i++) {
	    if (it_ == itend_) {
		wb_ = it_;
		return;
	    }
	    ++it_;
	}
    }

    bool operator==(const window_range_iter& o) const {
	return (it_ == o.it_ && n_ == o.n_);
    }

    bool operator!=(const window_range_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const window_iter_end&) const {
	return wb_ == itend_;
    }

    bool operator!=(const window_iter_end& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	if (it_ != itend_) {
	    ++it_;
	    ++wb_;
	} else {
	    wb_ = it_;
	}
	return *this;
    }

    auto operator*() {
	return range(wb_, it_);
    }
};


// iterator : window as list

template <typename Iter, typename Iterend>
class window_list_iter {
private:
    using val_type = std::remove_reference_t<decltype(*std::declval<Iter>())>;

    Iter it_;
    Iterend itend_;
    size_t n_ = 0;
    std::list<val_type> window_;

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= val_type;
    using pointer		= value_type*;
    using reference		= value_type&;

    window_list_iter() = default;
    window_list_iter(const window_list_iter&) = default;
    window_list_iter(window_list_iter&&) = default;
    window_list_iter& operator=(const window_list_iter&) = default;
    window_list_iter& operator=(window_list_iter&&) = default;

    window_list_iter(Iter it, Iterend itend, size_t n): it_(it), itend_(itend), n_(n) {
	for (size_t i = 0; i < n; i++) {
	    if (it_ == itend_) {
		window_.clear();
		return;
	    }
	    window_.push_back(std::forward<decltype(*it_)>(*it_));
	    ++it_;
	}
    }

    bool operator==(const window_list_iter& o) const {
	return (it_ == o.it_ && n_ == o.n_);
    }

    bool operator!=(const window_list_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const window_iter_end&) const {
	return window_.empty();
    }

    bool operator!=(const window_iter_end& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	if (it_ != itend_) {
	    window_.pop_front();
	    window_.push_back(std::forward<decltype(*it_)>(*it_));
	    ++it_;
	} else {
	    window_.clear();
	}
	return *this;
    }

    auto& operator*() {
	return window_;
    }
};


// window_seq object

template <typename Seq,
	  template <typename, typename> class WindowIter>
class window_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    size_t n_;

public:
    window_seq(Seq seq, size_t n):
	seq_(std::forward<Seq>(seq)), n_(n) {
    }

    window_seq(const window_seq&) = delete;
    window_seq& operator=(const window_seq&) = delete;
    window_seq(window_seq&&) = default;
    window_seq& operator=(window_seq&&) = delete;

    auto size() const {
	return seq_.size();
    }

    auto begin() {
	using std::begin;
	using std::end;
	auto it = begin(seq_);
	auto itend = end(seq_);
	return WindowIter<decltype(it), decltype(itend)>(it, itend, n_);
    }

    auto end() {
	return window_iter_end{};
    }

    auto begin() const {
	return const_cast<window_seq<Seq, WindowIter>*>(this)->begin();
    }

    auto end() const {
	return window_iter_end{};
    }

    auto rbegin() {
	using std::rbegin;
	using std::rend;
	auto it = rbegin(seq_);
	auto itend = rend(seq_);
	return WindowIter<decltype(it), decltype(itend)>(it, itend, n_);
    }

    auto rend() {
	return window_iter_end{};
    }

    auto rbegin() const {
	return const_cast<window_seq<Seq, WindowIter>*>(this)->rbegin();
    }

    auto rend() const {
	return window_iter_end{};
    }
};


// factory

class window_list {
public:
    explicit window_list(size_t n): n_(n) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return window_seq<decltype(seq), window_list_iter>(std::forward<Seq>(seq), n_);
    }

private:
    size_t n_ = 0;
};


class window_range {
public:
    explicit window_range(size_t n): n_(n) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return window_seq<decltype(seq), window_range_iter>(std::forward<Seq>(seq), n_);
    }

private:
    size_t n_ = 0;
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename Seq,
	  template <typename, typename> class WindowIter>
struct format_ident<c7::nseq::window_seq<Seq, WindowIter>> {
    static constexpr const char *name = "window";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/window.hpp
