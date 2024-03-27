/*
 * c7nseq/sort.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_SORT_HPP_LOADED_
#define C7_NSEQ_SORT_HPP_LOADED_


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


template <typename Seq, typename Compare>
class sort_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    Compare cmp_;

public:
    sort_seq(Seq seq, Compare cmp):
	seq_(std::forward<Seq>(seq)), cmp_(cmp) {
	using std::begin;
	auto b = begin(seq_);
	std::sort(b, b + seq_.size(), cmp);
    }

    sort_seq(const sort_seq&) = delete;
    sort_seq& operator=(const sort_seq&) = delete;
    sort_seq(sort_seq&&) = default;
    sort_seq& operator=(sort_seq&&) = delete;

    auto size() const {
	return seq_.size();
    }

    auto begin() {
	using std::begin;
	return begin(seq_);
    }

    auto end() {
	using std::end;
	return end(seq_);
    }

    auto begin() const {
	using std::begin;
	return begin(seq_);
    }

    auto end() const {
	using std::end;
	return end(seq_);
    }

    auto rbegin() {
	using std::rbegin;
	return rbegin(seq_);
    }

    auto rend() {
	using std::rend;
	return rend(seq_);
    }

    auto rbegin() const {
	using std::rbegin;
	return rbegin(seq_);
    }

    auto rend() const {
	using std::rend;
	return rend(seq_);
    }
};


template <typename Compare>
class sort_by {
public:
    explicit sort_by(Compare cmp): cmp_(cmp) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return sort_seq<decltype(seq), Compare>(std::forward<Seq>(seq), cmp_);
    }

private:
    Compare cmp_;
};


class sort {
public:
    template <typename Seq>
    auto operator()(Seq&& seq) {
	using std::begin;
	using item_type = typename std::iterator_traits<decltype(begin(seq))>::value_type;
	auto cmp = std::less<item_type>{};
	return sort_seq<decltype(seq), decltype(cmp)>(std::forward<Seq>(seq), cmp);
    }
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <typename Seq, typename Compare>
struct format_ident<c7::nseq::sort_seq<Seq, Compare>> {
    static constexpr const char *name = "sort";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/sort.hpp
