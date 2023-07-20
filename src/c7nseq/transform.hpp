/*
 * c7nseq/transform.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ_TRANSFORM_HPP_LOADED__
#define C7_NSEQ_TRANSFORM_HPP_LOADED__


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


// transform

struct transform_iter_end {};


template <typename Iter, typename Iterend, typename UnaryOperator>
class transform_iter {
private:
    Iter it_;
    Iterend itend_;
    UnaryOperator op_;

public:
    // for STL iterator
    using iterator_category	= typename std::iterator_traits<Iter>::iterator_category;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<decltype(op_(*it_))>;
    using pointer		= value_type*;
    using reference		= value_type&;

    transform_iter(Iter it, Iterend itend, UnaryOperator op): it_(it), itend_(itend), op_(op) {}

    bool operator==(const transform_iter& o) const {
	return (it_ == o.it_);
    }

    bool operator!=(const transform_iter& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	++it_;
	return *this;
    }

    decltype(auto) operator*() {
	return op_(*it_);
    }

    // for random access

    auto& operator--() {
	--it_;
	return *this;
    }

    decltype(auto) operator[](ptrdiff_t n) const {
	return op_(it_[n]);
    }

    auto& operator+=(ptrdiff_t n) {
	it_ += n;
	return *this;
    }

    auto operator+(ptrdiff_t n) const {
	return transform_iter(it_ + n, itend_, op_);
    }

    ptrdiff_t operator-(const transform_iter& o) const {
	return it_ - o.it_;
    }

    // introducing operators
#define C7_NSEQ_ITER_TYPE__	transform_iter
#define C7_NSEQ_ITEREND_TYPE__	transform_iter_end
#include <c7nseq/_iter_ops.hpp>
};


template <typename Seq, typename UnaryOperator>
class transform_obj {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    UnaryOperator op_;

public:
    transform_obj(Seq seq, UnaryOperator op):
	seq_(std::forward<Seq>(seq)), op_(op) {
    }

    transform_obj(const transform_obj&) = delete;
    transform_obj& operator=(const transform_obj&) = delete;
    transform_obj(transform_obj&&) = default;
    transform_obj& operator=(transform_obj&&) = delete;

    auto size() const {
	return seq_.size();
    }

    auto begin() {
	using std::begin;
	using std::end;
	auto it = begin(seq_);
	auto itend = end(seq_);
	return transform_iter<decltype(it), decltype(itend), UnaryOperator>(it, itend, op_);
    }

    auto end() {
	return transform_iter_end{};
    }

    auto begin() const {
	return const_cast<transform_obj<Seq, UnaryOperator>*>(this)->begin();
    }

    auto end() const {
	return transform_iter_end{};
    }

    auto rbegin() {
	using std::rbegin;
	using std::rend;
	auto it = rbegin(seq_);
	auto itend = rend(seq_);
	return transform_iter<decltype(it), decltype(itend), UnaryOperator>(it, itend, op_);
    }

    auto rend() {
	return transform_iter_end{};
    }

    auto rbegin() const {
	return const_cast<transform_obj<Seq, UnaryOperator>*>(this)->rbegin();
    }

    auto rend() const {
	return transform_iter_end{};
    }
};


template <typename UnaryOperator>
class transform {
public:
    explicit transform(UnaryOperator op): op_(op) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return transform_obj<decltype(seq), UnaryOperator>(std::forward<Seq>(seq), op_);
    }

private:
    UnaryOperator op_;
};


// unref

class unref {
public:
    template <typename Seq>
    auto operator()(Seq&& seq) {
	auto cv = [](auto v) {
	    return static_cast<std::remove_reference_t<decltype(v)>>(v);
	};
	return transform_obj<decltype(seq), decltype(cv)>(std::forward<Seq>(seq), cv);
    }
};


// ptr

class ptr {
public:
    template <typename Seq>
    auto operator()(Seq&& seq) {
	auto cv = [](auto& v) {
	    return &v;
	};
	return transform_obj<decltype(seq), decltype(cv)>(std::forward<Seq>(seq), cv);
    }
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename Seq, typename UnaryOperator>
struct format_ident<c7::nseq::transform_obj<Seq, UnaryOperator>> {
    static constexpr const char *name = "transform";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/transform.hpp
