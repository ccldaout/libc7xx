/*
 * c7nseq/flat.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ_FLAT_HPP_LOADED__
#define C7_NSEQ_FLAT_HPP_LOADED__


#include <c7nseq/generator.hpp>


namespace c7::nseq {


/*----------------------------------------------------------------------------
                                 N-level flat
----------------------------------------------------------------------------*/

template <int N, typename T>
struct flat_item {
    using sub_type = decltype(*std::begin(std::declval<T>()));
    using type = typename flat_item<N-1, sub_type>::type;
};

template <typename T>
struct flat_item<0, T> {
    using type = T;
};

template <int N, typename T>
using flat_item_t = typename flat_item<N, T>::type;

template <int N>
class flat {
private:
    size_t buffer_size_;

    template <int L, typename Seq, typename Out>
    static void traverse_inner(Seq&& seq, Out& out) {
	for (auto&& item: seq) {
	    using item_type = decltype(item);
	    if constexpr (L > 0) {
		traverse_inner<L-1>(std::forward<item_type>(item), out);
	    } else {
		out << std::forward<item_type>(item);
	    }
	}
    }

    template <typename Seq, typename Out>
    static void traverse_entry(Seq& seq, Out& out) {
	return traverse_inner<N>(seq, out);
    }

public:
    explicit flat(size_t buffer_size=1024): buffer_size_(buffer_size) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	using item_type = c7::typefunc::remove_cref_t<flat_item_t<N+1, Seq>>;
	return generator<item_type>(traverse_entry<Seq, co_output<item_type>>, buffer_size_)(
	    std::forward<Seq>(seq));
    }
};


/*----------------------------------------------------------------------------
                                  full flat
----------------------------------------------------------------------------*/

template <typename, bool>
struct flat0_item_impl;

template <typename T>
struct flat0_item_impl<T, true> {
    using item_type = decltype(*std::begin(std::declval<T>()));
    using type = typename flat0_item_impl<item_type,
					 c7::typefunc::is_iterable_v<item_type>>::type;
};

template <typename T>
struct flat0_item_impl<T, false> {
    using type = T;
};

template <typename T>
struct flat0_item {
    using type = typename flat0_item_impl<T,
					 c7::typefunc::is_iterable_v<T>>::type;
};

template <typename T>
using flat0_item_t = typename flat0_item<T>::type;

template <>
class flat<0> {
private:
    size_t buffer_size_;

    template <typename Seq, typename Out>
    static void traverse_inner(Seq&& seq, Out& out) {
	for (auto&& item: seq) {
	    using item_type = decltype(item);
	    if constexpr (c7::typefunc::is_iterable_v<item_type>) {
		traverse_inner(std::forward<item_type>(item), out);
	    } else {
		out << std::forward<item_type>(item);
	    }
	}
    }

    template <typename Seq, typename Out>
    static void traverse_entry(Seq& seq, Out& out) {
	return traverse_inner(seq, out);
    }

public:
    explicit flat(size_t buffer_size=1): buffer_size_(buffer_size) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	using item_type = c7::typefunc::remove_cref_t<flat0_item_t<Seq>>;
	return generator<item_type>(traverse_entry<Seq, co_output<item_type>>, buffer_size_)(
	    std::forward<Seq>(seq));
    }
};


} // namespace c7::nseq


#endif // c7nseq/flat.hpp
