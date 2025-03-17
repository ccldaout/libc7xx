/*
 * c7nseq/while.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_WHILE_HPP_LOADED_
#define C7_NSEQ_WHILE_HPP_LOADED_


#include <c7nseq/_cmn.hpp>
#include <c7nseq/filter.hpp>
#include <c7nseq/generator.hpp>


namespace c7::nseq {


struct take_while_iter_end {};


template <typename Iter, typename Iterend, typename Predicate>
class take_while_iter {
private:
    Iter it_;
    Iterend itend_;
    Predicate pred_;
    bool ended_ = false;

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    take_while_iter() = default;
    take_while_iter(const take_while_iter&) = default;
    take_while_iter(take_while_iter&&) = default;
    take_while_iter& operator=(const take_while_iter&) = default;
    take_while_iter& operator=(take_while_iter&&) = default;

    take_while_iter(Iter it, Iterend itend, Predicate pred): it_(it), itend_(itend), pred_(pred) {
	ended_ = (it_ == itend_) || !pred_(*it_);
    }

    auto& operator++() {
	if (!ended_) {
	    ++it_;
	    ended_ = (it_ == itend_) || !pred_(*it_);
	}
	return *this;
    }

    bool operator==(const take_while_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const take_while_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const take_while_iter_end&) const {
	return ended_;
    }

    bool operator!=(const take_while_iter_end& o) const {
	return !(*this == o);
    }

    decltype(auto) operator*() {
	return (*it_);
    }

    decltype(auto) operator*() const {
	return (*it_);
    }
};


template <typename Seq, typename Predicate>
class take_while_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    Predicate pred_;

public:
    take_while_seq(Seq seq, Predicate pred):
	seq_(std::forward<Seq>(seq)), pred_(pred) {
    }

    take_while_seq(const take_while_seq&) = delete;
    take_while_seq& operator=(const take_while_seq&) = delete;
    take_while_seq(take_while_seq&&) = default;
    take_while_seq& operator=(take_while_seq&&) = delete;

    auto begin() {
	using std::begin;
	using std::end;
	auto it = begin(seq_);
	auto itend = end(seq_);
	return take_while_iter<decltype(it), decltype(itend), Predicate>(it, itend, pred_);
    }

    auto end() {
	return take_while_iter_end{};
    }

    auto begin() const {
	return const_cast<take_while_seq<Seq, Predicate>*>(this)->begin();
    }

    auto end() const {
	return take_while_iter_end{};
    }
};


template <typename Predicate>
class take_while {
public:
    explicit take_while(Predicate pred): pred_(pred) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return take_while_seq<decltype(seq), Predicate>(std::forward<Seq>(seq), pred_);
    }

private:
    Predicate pred_;
};


template <typename Predicate>
class skip_while {
private:
    Predicate pred_;

public:
    explicit skip_while(Predicate pred): pred_(pred) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	bool available = false;
	auto wrapper =
	    [available, pred=pred_](const auto& v) mutable {
		if (!available) {
		    available = !pred(v);
		}
		return available;
	    };
	return filter(wrapper)(std::forward<Seq>(seq));
    }
};


/*----------------------------------------------------------------------------
                               dtop_tail_while
----------------------------------------------------------------------------*/

template <typename Predicate>
class drop_tail_while {
private:
    Predicate pred_;
    size_t buffer_size_;

    template <typename Seq, typename Output>
    void generate(Seq& seq, c7::nseq::co_output<Output>& out) {
	std::vector<Output> hold;
	for (auto&& v: seq) {
	    if (!pred_(v)) {
		out << hold;
		out << v;
		hold.clear();
	    } else {
		hold.push_back(std::forward<decltype(v)>(v));
	    }
	}
    }

public:
    explicit drop_tail_while(Predicate pred, size_t buffer_size=1024):
	pred_(pred), buffer_size_(buffer_size) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	using std::begin;
	using out_type = c7::typefunc::remove_cref_t<decltype(*begin(seq))>;

	return c7::nseq::generator<out_type>(
	    [this](auto& seq, auto& out) {
		generate<decltype(seq), out_type>(seq, out);
	    },
	    buffer_size_)(std::forward<Seq>(seq));
    }
};


/*----------------------------------------------------------------------------
                               take_tail_while
----------------------------------------------------------------------------*/

template <typename Predicate>
class take_tail_while {
private:
    Predicate pred_;
    size_t buffer_size_;

    template <typename Seq, typename Output>
    void generate(Seq& seq, c7::nseq::co_output<Output>& out) {
	std::vector<Output> hold;
	for (auto&& v: seq) {
	    if (!pred_(v)) {
		hold.clear();
	    } else {
		hold.push_back(std::forward<decltype(v)>(v));
	    }
	}
	out << hold;
    }

public:
    explicit take_tail_while(Predicate pred, size_t buffer_size=1024):
	pred_(pred), buffer_size_(buffer_size) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	using std::begin;
	using out_type = c7::typefunc::remove_cref_t<decltype(*begin(seq))>;

	return c7::nseq::generator<out_type>(
	    [this](auto&& seq, auto& out) {
		generate<decltype(seq), out_type>(seq, out);
	    },
	    buffer_size_)(std::forward<Seq>(seq));
    }
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <typename Seq, typename Predicate>
struct format_ident<c7::nseq::take_while_seq<Seq, Predicate>> {
    static constexpr const char *name = "take_while";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/while.hpp
