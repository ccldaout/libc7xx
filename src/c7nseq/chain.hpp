/*
 * c7nseq/chain.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ_CHAIN_HPP_LOADED__
#define C7_NSEQ_CHAIN_HPP_LOADED__


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


class chain_iter_end {};


template <typename It1, typename It1end,
	  typename It2, typename It2end>
class chain_iter {
private:
    It1    it1_;
    It1end it1end_;
    It2    it2_;
    It2end it2end_;
    int    which_ = 0;

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= typename It1::value_type;
    using pointer		= value_type*;
    using reference		= value_type&;

    chain_iter() = default;
    chain_iter(const chain_iter&) = default;
    chain_iter(chain_iter&&) = default;
    chain_iter& operator=(const chain_iter&) = default;
    chain_iter& operator=(chain_iter&&) = default;

    chain_iter(It1 it1, It1end it1end, It2 it2, It2end it2end):
	it1_(it1), it1end_(it1end), it2_(it2), it2end_(it2end) {
    }

    bool operator==(const chain_iter& o) const {
	return (it1_ == o.it1_ && it2_ == o.it2_);
    }

    bool operator!=(const chain_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const chain_iter_end&) const {
	return (it2_ == it2end_);
    }

    bool operator!=(const chain_iter_end& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	if (it1_ != it1end_) {
	    ++it1_;
	    if (it1_ == it1end_) {
		which_ = 1;
	    }
	} else if (it2_ != it2end_) {
	    ++it2_;
	}
	return *this;
    }

    decltype(auto) operator*() {
	if (which_ == 0) {
	    return *it1_;
	} else {
	    return *it2_;
	}
    }
};


template <typename Seq1, typename Seq2>
class chain_seq {
private:
    template <typename S>
    using get_hold_type_t =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<S>,
			       std::remove_reference_t<S>,
			       S>;

    get_hold_type_t<Seq1> seq1_;
    get_hold_type_t<Seq2> seq2_;

public:
    chain_seq(Seq1 seq1, Seq2 seq2):
	seq1_(std::forward<Seq1>(seq1)),
	seq2_(std::forward<Seq2>(seq2)) {
    }

    chain_seq(const chain_seq&) = delete;
    chain_seq& operator=(const chain_seq&) = delete;
    chain_seq(chain_seq&&) = default;
    chain_seq& operator=(chain_seq&&) = delete;

    auto begin() {
	using std::begin;
	using std::end;
	auto it1    = begin(seq1_);
	auto it1end = end  (seq1_);
	auto it2    = begin(seq2_);
	auto it2end = end  (seq2_);
	return chain_iter<decltype(it1), decltype(it1end),
			  decltype(it2), decltype(it2end)>(
			      it1, it1end, it2, it2end);
    }

    auto end() {
	return chain_iter_end{};
    }

    auto begin() const {
	return const_cast<chain_seq<Seq1, Seq2>*>(this)->begin();
    }

    auto end() const {
	return chain_iter_end{};
    }
};


template <typename... Seqs>
auto chain(Seqs&&... seqs)
{
    static_assert(sizeof...(Seqs) == 2);
    return chain_seq<Seqs...>(std::forward<Seqs>(seqs)...);
}


template <typename S1, typename S2,
	  typename = std::enable_if_t<std::conjunction_v<
					  c7::typefunc::is_iterable<S1>,
					  c7::typefunc::is_iterable<S2>>>>
decltype(auto) operator,(S1&& s1, S2&& s2)
{
    return chain(std::forward<S1>(s1), std::forward<S2>(s2));
}


template <typename S1, typename S2,
	  typename = std::enable_if_t<std::conjunction_v<
					  c7::typefunc::is_iterable<S1>,
					  c7::typefunc::is_iterable<S2>>>>
decltype(auto) operator+(S1&& s1, S2&& s2)
{
    return chain(std::forward<S1>(s1), std::forward<S2>(s2));
}


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename... Seqs>
struct format_ident<c7::nseq::chain_seq<Seqs...>> {
    static constexpr const char *name = "chain";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/chain.hpp
