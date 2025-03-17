/*
 * c7nseq/infix.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_INFIX_HPP_LOADED_
#define C7_NSEQ_INFIX_HPP_LOADED_


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


struct infix_iter_end {};


template <typename Iter, typename Iterend, typename BinaryOp>
class infix_iter {
public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*std::declval<Iter>())>;
    using pointer		= value_type*;
    using reference		= value_type&;

    infix_iter() = default;
    infix_iter(const infix_iter&) = default;
    infix_iter(infix_iter&&) = default;
    infix_iter& operator=(const infix_iter&) = default;
    infix_iter& operator=(infix_iter&&) = default;

    infix_iter(Iter it, Iterend itend, BinaryOp op): it_(it), itend_(itend), op_(op) {
    }

    bool operator==(const infix_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const infix_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const infix_iter_end&) const {
	return (it_ == itend_);
    }

    bool operator!=(const infix_iter_end& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	if (state_ == state::opr) {
	    if (it_ != itend_) {
		auto opr = *it_;
		++it_;
		if (it_ != itend_) {
		    infix_ = op_(opr, *it_);
		    state_ = state::infix;
		}
	    }
	} else {
	    state_ = state::opr;
	}
	return *this;
    }

    decltype(auto) operator*() {
	if (state_ == state::infix) {
	    return infix_;
	} else {
	    return (*it_);
	}
    }

    decltype(auto) operator*() const {
	if (state_ == state::infix) {
	    return infix_;
	} else {
	    return (*it_);
	}
    }

private:
    Iter it_;
    Iterend itend_;
    BinaryOp op_;

    enum class state { opr, infix };
    state state_ = state::opr;
    value_type infix_;
};


template <typename Seq, typename BinaryOp>
class infix_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    BinaryOp op_;

public:
    infix_seq(Seq seq, BinaryOp op):
	seq_(std::forward<Seq>(seq)), op_(op) {
    }

    infix_seq(const infix_seq&) = delete;
    infix_seq& operator=(const infix_seq&) = delete;
    infix_seq(infix_seq&&) = default;
    infix_seq& operator=(infix_seq&&) = delete;

    auto begin() {
	using std::begin;
	using std::end;
	auto it = begin(seq_);
	auto itend = end(seq_);
	return infix_iter<decltype(it), decltype(itend), BinaryOp>(it, itend, op_);
    }

    auto end() {
	return infix_iter_end{};
    }

    auto begin() const {
	return const_cast<infix_seq<Seq, BinaryOp>*>(this)->begin();
    }

    auto end() const {
	return infix_iter_end{};
    }
};


template <typename BinaryOp>
class infix {
public:
    explicit infix(BinaryOp op): op_(op) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return infix_seq<decltype(seq), BinaryOp>(std::forward<Seq>(seq), op_);
    }

private:
    BinaryOp op_;
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <typename Seq, typename BinaryOp>
struct format_ident<c7::nseq::infix_seq<Seq, BinaryOp>> {
    static constexpr const char *name = "infix";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/infix.hpp
