/*
 * c7nseq/filter.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ_FILTER_HPP_LOADED__
#define C7_NSEQ_FILTER_HPP_LOADED__


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


struct filter_iter_end {};


template <typename Iter, typename Iterend, typename Predicate>
class filter_iter {
private:
    Iter it_;
    Iterend itend_;
    Predicate pred_;

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    filter_iter(Iter it, Iterend itend, Predicate pred): it_(it), itend_(itend), pred_(pred) {
	while (it_ != itend_) {
	    if (pred_(*it_)) {
		break;
	    }
	    ++it_;
	}
    }

    bool operator==(const filter_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const filter_iter& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	if (it_ != itend_) {
	    ++it_;
	    while (it_ != itend_) {
		if (pred_(*it_)) {
		    break;
		}
		++it_;
	    }
	}
	return *this;
    }

    decltype(auto) operator*() {
	return (*it_);
    }

    decltype(auto) operator*() const {
	return (*it_);
    }

    // deduced from aboves

    bool operator==(const filter_iter_end&) const {
	return (it_ == itend_);
    }

    bool operator!=(const filter_iter_end& o) const {
	return !(*this == o);
    }
};


template <typename Seq, typename Predicate>
class filter_obj {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    Predicate pred_;

public:
    filter_obj(Seq seq, Predicate pred):
	seq_(std::forward<Seq>(seq)), pred_(pred) {
    }

    filter_obj(const filter_obj&) = delete;
    filter_obj& operator=(const filter_obj&) = delete;
    filter_obj(filter_obj&&) = default;
    filter_obj& operator=(filter_obj&&) = delete;

    auto begin() {
	using std::begin;
	using std::end;
	auto it = begin(seq_);
	auto itend = end(seq_);
	return filter_iter<decltype(it), decltype(itend), Predicate>(it, itend, pred_);
    }

    auto end() {
	return filter_iter_end{};
    }

    auto begin() const {
	return const_cast<filter_obj<Seq, Predicate>*>(this)->begin();
    }

    auto end() const {
	return filter_iter_end{};
    }

    auto rbegin() {
	using std::rbegin;
	using std::rend;
	auto it = rbegin(seq_);
	auto itend = rend(seq_);
	return filter_iter<decltype(it), decltype(itend), Predicate>(it, itend, pred_);
    }

    auto rend() {
	return filter_iter_end{};
    }

    auto rbegin() const {
	return const_cast<filter_obj<Seq, Predicate>*>(this)->rbegin();
    }

    auto rend() const {
	return filter_iter_end{};
    }

};


template <typename Predicate>
class filter {
public:
    explicit filter(Predicate pred): pred_(pred) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return filter_obj<decltype(seq), Predicate>(std::forward<Seq>(seq), pred_);
    }

private:    
    Predicate pred_;
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename Seq, typename Predicate>
struct format_ident<c7::nseq::filter_obj<Seq, Predicate>> {
    static constexpr const char *name = "filter";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/filter.hpp
