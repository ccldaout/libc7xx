/*
 * c7seq.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=637771050
 */
#ifndef C7_SEQ_HPP_LOADED__
#define C7_SEQ_HPP_LOADED__
#include <c7common.hpp>


#include <cstdio>
#include <functional>
#include <iterator>	// std::iterator_traits
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <c7utils.hpp>	// c_array_iterator


#include <c7nseq/c_array.hpp>
#include <c7nseq/enumerate.hpp>
#include <c7nseq/filter.hpp>
#include <c7nseq/head.hpp>
#include <c7nseq/range.hpp>
#include <c7nseq/reverse.hpp>
#include <c7nseq/tail.hpp>
#include <c7nseq/transform.hpp>


namespace c7::seq {


using c7::nseq::range;

template <typename C>
auto head(C&& c, size_t len)
{
    return std::forward<C>(c) | c7::nseq::head(len);
}

template <typename C>
auto tail(C&& c, size_t len)
{
    return std::forward<C>(c) | c7::nseq::skip_head(len);
}

template <typename C>
auto reverse(C&& c)
{
    return std::forward<C>(c) | c7::nseq::reverse();
}

template <typename C>
auto enumerate(C&& c, ssize_t index = 0)
{
    return std::forward<C>(c) | c7::nseq::enumerate(index);
}

template <typename C, typename F>
auto convert(C&& c, F conv)
{
    return std::forward<C>(c) | c7::nseq::transform(conv);
}

template <typename C>
auto ptr(C&& c)
{
    return std::forward<C>(c) | c7::nseq::ptr();
}

template <typename C, typename Predicate>
auto filter(C&& c, Predicate pred)
{
    return std::forward<C>(c) | c7::nseq::filter(pred);
}

using c7::nseq::c_array;

template <typename T>
auto c_parray(T *array)
{
    size_t n;
    for (n = 0; array[n] != nullptr; n++);
    return c7::nseq::c_array_obj<T>(n, array);
}

template <typename T, typename UnaryOperator>
auto c_parray(T *array, UnaryOperator op)
{
    return c_parray(array) | c7::nseq::transform(op);
}

template <typename Char>
auto charp_array(Char **array)
{
    return c_parray(array, [](Char *p){ return std::string(p); });
}


} // namespace c7::seq


#endif // c7seq.hpp
