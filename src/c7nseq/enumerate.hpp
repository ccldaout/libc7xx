/*
 * c7nseq/enumerate.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_ENUMERATE_HPP_LOADED_
#define C7_NSEQ_ENUMERATE_HPP_LOADED_


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


struct enumerate_iter_end {};


template <typename Iter, typename Iterend>
class enumerate_iter {
private:
    Iter it_;
    Iterend itend_;
    int i_, step_;

public:
    // for STL iterator
    using iterator_category	= typename std::iterator_traits<Iter>::iterator_category;
    using difference_type	= ptrdiff_t;
    using value_type		= std::pair<int, decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    enumerate_iter() = default;
    enumerate_iter(const enumerate_iter&) = default;
    enumerate_iter(enumerate_iter&&) = default;
    enumerate_iter& operator=(const enumerate_iter&) = default;
    enumerate_iter& operator=(enumerate_iter&&) = default;

    enumerate_iter(Iter it, Iterend itend, int i, int step):
	it_(it), itend_(itend), i_(i), step_(step) {}

    bool operator==(const enumerate_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const enumerate_iter& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	++it_;
	i_ += step_;
	return *this;
    }

    decltype(auto) operator*() {
	return std::pair<int, decltype(*it_)>(i_, *it_);
    }

    decltype(auto) operator*() const {
	return std::pair<int, decltype(*it_)>(i_, *it_);
    }

    // for random access

    auto& operator--() {
	--it_;
	i_ -= step_;
	return *this;
    }

    decltype(auto) operator[](ptrdiff_t n) const {
	return std::pair<int, decltype(*it_)>(i_ + n * step_, it_[n]);
    }

    auto& operator+=(ptrdiff_t n) {
	it_ += n;
	i_  += (n * step_);
	return *this;
    }

    auto operator+(ptrdiff_t n) const {
	return enumerate_iter(it_ + n, itend_, i_ + (n * step_), step_);
    }

    ptrdiff_t operator-(const enumerate_iter& o) const {
	return (it_ - o.it_) / step_;
    }

    // introducing operators
#define C7_NSEQ_ITER_TYPE_	enumerate_iter
#define C7_NSEQ_ITEREND_TYPE_	enumerate_iter_end
#include <c7nseq/_iter_ops.hpp>
};


template <typename Seq>
class enumerate_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    int beg_, step_;

public:
    enumerate_seq(Seq seq, int beg, int step):
	seq_(std::forward<Seq>(seq)), beg_(beg), step_(step) {
    }

    enumerate_seq(const enumerate_seq&) = delete;
    enumerate_seq& operator=(const enumerate_seq&) = delete;
    enumerate_seq(enumerate_seq&&) = default;
    enumerate_seq& operator=(enumerate_seq&&) = delete;

    auto size() const {
	return seq_.size();
    }

    auto begin() {
	using std::begin;
	using std::end;
	auto it = begin(seq_);
	auto itend = end(seq_);
	return enumerate_iter<decltype(it), decltype(itend)>(it, itend, beg_, step_);
    }

    auto end() {
	return enumerate_iter_end{};
    }

    auto begin() const {
	return const_cast<enumerate_seq<Seq>*>(this)->begin();
    }

    auto end() const {
	return enumerate_iter_end{};
    }

    auto rbegin() {
	using std::rbegin;
	using std::rend;
	auto it = rbegin(seq_);
	auto itend = rend(seq_);
	int beg = beg_ + step_ * (static_cast<int>(seq_.size()) - 1);
	int step = -step_;
	return enumerate_iter<decltype(it), decltype(itend)>(it, itend, beg, step);
    }

    auto rend() {
	return enumerate_iter_end{};
    }

    auto rbegin() const {
	return const_cast<enumerate_seq<Seq>*>(this)->rbegin();
    }

    auto rend() const {
	return enumerate_iter_end{};
    }
};


class enumerate {
public:
    explicit enumerate(int beg=0, int step=1):
	beg_(beg), step_(step) {
    }

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return enumerate_seq<decltype(seq)>(std::forward<Seq>(seq), beg_, step_);
    }

private:
    int beg_, step_;
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <typename Seq>
struct format_ident<c7::nseq::enumerate_seq<Seq>> {
    static constexpr const char *name = "enumerate";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/enumerate.hpp
