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
#ifndef C7_NSEQ_STORE_ACCUMULATE_HPP_LOADED__
#define C7_NSEQ_STORE_ACCUMULATE_HPP_LOADED__


#include <numeric>
#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


template <typename BinaryOperation, typename T>
class accumulate_with_init {
private:
    BinaryOperation op_;
    T val_;

public:
    accumulate_with_init(BinaryOperation op, T init): op_(op), val_(init) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	auto it = std::begin(seq);
	auto end = std::end(seq);
	auto a = val_;
	for (; it != end; ++it) {
	    a = op_(a, *it);
	}
	return a;
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
	auto it = std::begin(seq);
	auto end = std::end(seq);
	auto a = *it;
	for (++it; it != end; ++it) {
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


} // namespace c7::nseq


#endif // c7nseq/accumulate.hpp
