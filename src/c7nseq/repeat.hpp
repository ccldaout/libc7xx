/*
 * c7nseq/repeat.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_REPEAT_HPP_LOADED_
#define C7_NSEQ_REPEAT_HPP_LOADED_


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


class repeat_iter_end {};


// repeat N sequence -> sequence

template <typename Seq>
class repeat_iter {
private:
    using seq_type = std::remove_reference_t<Seq>;

    seq_type *seq_ = nullptr;
    size_t n_ = 0;

    static auto call_begin(seq_type s) {
	using std::begin;
	return begin(s);
    }
    static auto call_end(seq_type s) {
	using std::end;
	return end(s);
    }

    decltype(call_begin(std::declval<seq_type>())) it_;
    decltype(call_end(std::declval<seq_type>()))   itend_;

    void setup_iter() {
	using std::begin;
	using std::end;
	it_    = begin(*seq_);
	itend_ = end(*seq_);
    }

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    repeat_iter() = default;
    repeat_iter(const repeat_iter&) = default;
    repeat_iter(repeat_iter&&) = default;
    repeat_iter& operator=(const repeat_iter&) = default;
    repeat_iter& operator=(repeat_iter&&) = default;

    repeat_iter(Seq& seq, size_t n):
	seq_(&seq), n_(n) {
	setup_iter();
    }

    bool operator==(const repeat_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const repeat_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const repeat_iter_end&) const {
	return (n_ == 0 && it_ == itend_);
    }

    bool operator!=(const repeat_iter_end& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	if (++it_ == itend_) {
	    if (n_ > 0) {
		n_--;
	    }
	    if (n_ != 0) {
		setup_iter();
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
class repeat_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;

    hold_type seq_;
    size_t n_;

public:
    repeat_seq(Seq seq, size_t n):
	seq_(std::forward<Seq>(seq)), n_(n) {
    }

    repeat_seq(const repeat_seq&) = delete;
    repeat_seq& operator=(const repeat_seq&) = delete;
    repeat_seq(repeat_seq&&) = default;
    repeat_seq& operator=(repeat_seq&&) = delete;

    auto begin() {
	return repeat_iter<hold_type>(seq_, n_);
    }

    auto end() {
	return repeat_iter_end{};
    }

    auto begin() const {
	return const_cast<repeat_seq<Seq>*>(this)->begin();
    }

    auto end() const {
	return repeat_iter_end{};
    }
};


class repeat {
public:
    explicit repeat(size_t n = -1UL): n_(n) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return repeat_seq<decltype(seq)>(std::forward<Seq>(seq), n_);
    }

private:
    size_t n_;
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <typename Seq>
struct format_ident<c7::nseq::repeat_seq<Seq>> {
    static constexpr const char *name = "repeat";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/repeat.hpp
