/*
 * c7nseq/accumulate.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
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
	return std::accumulate(std::begin(seq), std::end(seq), val_, op_);
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
	auto beg = std::begin(seq);
	auto init = *beg;
	return std::accumulate(++beg, std::end(seq), init, op_);
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
