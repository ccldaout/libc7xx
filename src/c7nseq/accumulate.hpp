/*
 * c7nseq/accumulate.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_STORE_ACCUMULATE_HPP_LOADED_
#define C7_NSEQ_STORE_ACCUMULATE_HPP_LOADED_


#include <numeric>
#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


template <typename InplaceBinaryOperation, typename T>
class accumulate_inplace {
private:
    template <typename U>
    struct is_mutable_lvalue_reference {
	static inline constexpr bool value =
	    std::conjunction_v<std::is_lvalue_reference<U>,
			       std::negation<std::is_const<std::remove_reference_t<U>>>>;
    };

    using hold_type =
	std::conditional_t<is_mutable_lvalue_reference<T>::value,
			   T,
			   c7::typefunc::remove_cref_t<T>>;

    InplaceBinaryOperation op_;
    hold_type val_;

public:
    accumulate_inplace(InplaceBinaryOperation op, T init):
	op_(op), val_(std::forward<T>(init)) {}

    template <typename Seq>
    decltype(auto) operator()(Seq&& seq) {
	using std::begin;
	using std::end;
	auto it = begin(seq);
	auto itend = end(seq);
	for (; it != itend; ++it) {
	    op_(val_, *it);
	}
	return val_;
    }
};


template <typename BinaryOperation, typename T>
class accumulate_with_init {
private:
    BinaryOperation op_;
    T val_;

public:
    accumulate_with_init(BinaryOperation op, const T& init): op_(op), val_(init) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	using std::begin;
	using std::end;
	auto it = begin(seq);
	auto itend = end(seq);
	for (; it != itend; ++it) {
	    val_ = op_(val_, *it);
	}
	return val_;
    }
};


template <typename BinaryOperation>
class accumulate_without_init {
private:
    BinaryOperation op_;

public:
    accumulate_without_init(BinaryOperation op): op_(op) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	using std::begin;
	using std::end;
	auto it = begin(seq);
	auto itend = end(seq);
	auto a = *it;
	for (++it; it != itend; ++it) {
	    a = op_(a, *it);
	}
	return a;
    }
};


template <typename BinaryOperation>
auto accumulate(BinaryOperation op)
{
    return accumulate_without_init(op);
}


template <typename BinaryOperation, typename T>
auto accumulate(BinaryOperation op, T&& init)
{
    return accumulate_with_init(op, std::forward<T>(init));
}


template <typename InplaceBinaryOperation, typename T>
decltype(auto) accumulate_i(InplaceBinaryOperation op, T&& init)
{
    return accumulate_inplace<InplaceBinaryOperation, T>(op, std::forward<T>(init));
}


} // namespace c7::nseq


#endif // c7nseq/accumulate.hpp
