/*
 * c7nseq/group.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_GROUP_HPP_LOADED__
#define C7_NSEQ_GROUP_HPP_LOADED__


#include <c7nseq/_cmn.hpp>
#include <c7nseq/range.hpp>


namespace c7::nseq {


struct group_iter_end {};


// iterator : group as range with iterator pair

template <typename Iter, typename Iterend, typename Equal>
class group_range_iter {
private:
    Iter it_;
    Iterend itend_;
    Equal eq_;

    Iter group_beg_;
    Iter group_end_;

    void make_group() {
	group_beg_ = group_end_ = it_;
	if (it_ == itend_) {
	    return;
	}
	for (++it_; it_ != itend_; ++it_) {
	    if (!eq_(*group_beg_, *it_)) {
		break;
	    }
	}
	group_end_ = it_;
    }

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= decltype(range(group_beg_, group_end_));
    using pointer		= value_type*;
    using reference		= value_type&;

    group_range_iter() = default;
    group_range_iter(const group_range_iter&) = default;
    group_range_iter(group_range_iter&&) = default;
    group_range_iter& operator=(const group_range_iter&) = default;
    group_range_iter& operator=(group_range_iter&&) = default;

    group_range_iter(Iter it, Iterend itend, Equal eq): it_(it), itend_(itend), eq_(eq) {
	make_group();
    }

    bool operator==(const group_range_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const group_range_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const group_iter_end&) const {
	return (group_beg_ == group_end_);
    }

    bool operator!=(const group_iter_end& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	make_group();
	return *this;
    }

    auto operator*() {
	return range(group_beg_, group_end_);
    }
};


// iterator : group as vector

template <typename Iter, typename Iterend, typename Equal>
class group_vec_iter {
private:
    using val_type = std::remove_reference_t<decltype(*std::declval<Iter>())>;

    Iter it_;
    Iterend itend_;
    Equal eq_;
    std::vector<val_type> group_;

    void make_group() {
	group_.clear();
	if (it_ == itend_) {
	    return;
	}
	group_.push_back(std::forward<decltype(*it_)>(*it_));
	for (++it_; it_ != itend_; ++it_) {
	    if (!eq_(group_[0], *it_)) {
		break;
	    }
	    group_.push_back(std::forward<decltype(*it_)>(*it_));
	}
    }

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= val_type;
    using pointer		= value_type*;
    using reference		= value_type&;

    group_vec_iter() = default;
    group_vec_iter(const group_vec_iter&) = default;
    group_vec_iter(group_vec_iter&&) = default;
    group_vec_iter& operator=(const group_vec_iter&) = default;
    group_vec_iter& operator=(group_vec_iter&&) = default;

    group_vec_iter(Iter it, Iterend itend, Equal eq): it_(it), itend_(itend), eq_(eq) {
	make_group();
    }

    bool operator==(const group_vec_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const group_vec_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const group_iter_end&) const {
	return group_.empty();
    }

    bool operator!=(const group_iter_end& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	make_group();
	return *this;
    }

    decltype(auto) operator*() {
	return std::move(group_);
    }
};


// group_by object

template <typename Seq, typename Equal,
	  template <typename, typename, typename> class GroupIter>
class group_by_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    Equal eq_;

public:
    group_by_seq(Seq seq, Equal eq):
	seq_(std::forward<Seq>(seq)), eq_(eq) {
    }

    group_by_seq(const group_by_seq&) = delete;
    group_by_seq& operator=(const group_by_seq&) = delete;
    group_by_seq(group_by_seq&&) = default;
    group_by_seq& operator=(group_by_seq&&) = delete;

    auto size() const {
	return seq_.size();
    }

    auto begin() {
	using std::begin;
	using std::end;
	auto it = begin(seq_);
	auto itend = end(seq_);
	return GroupIter<decltype(it), decltype(itend), Equal>(it, itend, eq_);
    }

    auto end() {
	return group_iter_end{};
    }

    auto begin() const {
	return const_cast<group_by_seq<Seq, Equal, GroupIter>*>(this)->begin();
    }

    auto end() const {
	return group_iter_end{};
    }

    auto rbegin() {
	using std::rbegin;
	using std::rend;
	auto it = rbegin(seq_);
	auto itend = rend(seq_);
	return GroupIter<decltype(it), decltype(itend), Equal>(it, itend, eq_);
    }

    auto rend() {
	return group_iter_end{};
    }

    auto rbegin() const {
	return const_cast<group_by_seq<Seq, Equal, GroupIter>*>(this)->rbegin();
    }

    auto rend() const {
	return group_iter_end{};
    }
};


// factory

template <typename Equal>
class group_by {
public:
    explicit group_by(Equal eq): eq_(eq) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return group_by_seq<decltype(seq), Equal, group_vec_iter>(std::forward<Seq>(seq), eq_);
    }

private:
    Equal eq_;
};


template <typename Equal>
class group_range_by {
public:
    explicit group_range_by(Equal eq): eq_(eq) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return group_by_seq<decltype(seq), Equal, group_range_iter>(std::forward<Seq>(seq), eq_);
    }

private:
    Equal eq_;
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename Seq, typename Equal,
	  template <typename, typename, typename> class GroupIter>
struct format_ident<c7::nseq::group_by_seq<Seq, Equal, GroupIter>> {
    static constexpr const char *name = "group";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/group.hpp
