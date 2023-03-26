/*
 * c7nseq/reverse.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ_REVERSE_HPP_LOADED__
#define C7_NSEQ_REVERSE_HPP_LOADED__


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


struct reverse_iter_end {};


template <typename Iter, typename Iterend>
class reverse_iter {
private:
    Iter it_;
    Iterend itend_;

public:
    // for STL iterator
    using iterator_category	= typename std::iterator_traits<Iter>::iterator_category;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(*it_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    reverse_iter(Iter it, Iterend itend): it_(it), itend_(itend) {
    }

    bool operator==(const reverse_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const reverse_iter& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	++it_;
	return *this;
    }

    decltype(auto) operator*() {
	return *it_;
    }

    decltype(auto) operator*() const {
	return *it_;
    }

    // for random access

    auto& operator--() {
	--it_;
	return *this;
    }

    decltype(auto) operator[](ptrdiff_t n) const {
	return it_[n];
    }

    auto& operator+=(ptrdiff_t n) {
	it_ += n;
	return *this;
    }

    auto operator+(ptrdiff_t n) const {
	return reverse_iter(it_ + n, itend_);
    }

    ptrdiff_t operator-(const reverse_iter& o) const {
	return it_ - o.it_;
    }

    auto operator->() const {
	return &(*it_);
    }

    // introducing operators
#define C7_NSEQ_ITER_TYPE__	reverse_iter
#define C7_NSEQ_ITEREND_TYPE__	reverse_iter_end
#include <c7nseq/_iter_ops.hpp>
};


template <typename Seq>
class reverse_obj {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;

public:
    explicit reverse_obj(Seq seq):
	seq_(std::forward<Seq>(seq)) {
    }

    reverse_obj(const reverse_obj&) = delete;
    reverse_obj& operator=(const reverse_obj&) = delete;
    reverse_obj(reverse_obj&&) = default;
    reverse_obj& operator=(reverse_obj&&) = delete;

    auto size() const {
	return seq_.size();
    }

    auto begin() {
	using std::rbegin;
	using std::rend;
	auto it = rbegin(seq_);
	auto itend = rend(seq_);
	return reverse_iter<decltype(it), decltype(itend)>(it, itend);
    }

    auto end() {
	return reverse_iter_end{};
    }

    auto begin() const {
	return const_cast<reverse_obj<Seq>*>(this)->begin();
    }

    auto end() const {
	return reverse_iter_end{};
    }
};


class reverse {
public:
    template <typename Seq>
    auto operator()(Seq&& seq) {
	return reverse_obj<decltype(seq)>(std::forward<Seq>(seq));
    }
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename Seq>
struct format_ident<c7::nseq::reverse_obj<Seq>> {
    static constexpr const char *name = "reverse";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/reverse.hpp
