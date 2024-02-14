/*
 * c7nseq/zip.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ_ZIP_HPP_LOADED__
#define C7_NSEQ_ZIP_HPP_LOADED__


#include <c7nseq/_cmn.hpp>
#include <c7nseq/tuple.hpp>


namespace c7::nseq {


class zip_iter_end {};


// zip N sequence -> sequence of std::tuple

template <template <typename> class, typename>
struct t_apply {};
template <template <typename> class F, typename... Ts>
struct t_apply<F, std::tuple<Ts...>> {
    using type = std::tuple<F<Ts>...>;
};


template <typename ItsTuple, typename ItendsTuple>
class zipN_iter {
private:
    template <typename Iter>
    using get_value_type_t = typename std::iterator_traits<Iter>::value_type;

    ItsTuple its_;
    ItendsTuple itends_;

    template <int N>
    bool reach_end() const {
	if (std::get<N>(its_) == std::get<N>(itends_)) {
	    return true;
	}
	if constexpr (N == 0) {
	    return false;
        } else {
	    return reach_end<N-1>();
	}
    }

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= typename t_apply<get_value_type_t, ItsTuple>::type;
    using pointer		= value_type*;
    using reference		= value_type&;

    zipN_iter() = default;
    zipN_iter(const zipN_iter&) = default;
    zipN_iter(zipN_iter&&) = default;
    zipN_iter& operator=(const zipN_iter&) = default;
    zipN_iter& operator=(zipN_iter&&) = default;

    zipN_iter(ItsTuple iters, ItendsTuple iterends):
	its_(iters), itends_(iterends) {
    }

    bool operator==(const zipN_iter& o) const {
	return its_ == o.its_;
    }

    bool operator!=(const zipN_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const zip_iter_end&) const {
	constexpr int n = c7::typefunc::count_of_tuple_v<ItsTuple>;
	return reach_end<n-1>();
    }

    bool operator!=(const zip_iter_end& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	tuple_for_each(its_, [](auto& it){ ++it; });
	return *this;
    }

    auto operator*() {
	return tuple_transform(its_, [](auto& it) { return *it; });
    }
};


template <typename... Seqs>
class zipN_seq {
private:
    template <typename S>
    using get_hold_type_t =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<S>,
			       std::remove_reference_t<S>,
			       S>;

    std::tuple<get_hold_type_t<Seqs>...> seqs_;

public:
    zipN_seq(Seqs... seqs): seqs_(std::forward<Seqs>(seqs)...) {}

    zipN_seq(const zipN_seq&) = delete;
    zipN_seq& operator=(const zipN_seq&) = delete;
    zipN_seq(zipN_seq&&) = default;
    zipN_seq& operator=(zipN_seq&&) = delete;

    auto begin() {
	using std::begin;
	using std::end;
	auto its	= tuple_transform(seqs_, [](auto& s) { return begin(s); });
	auto itends	= tuple_transform(seqs_, [](auto& s) { return end  (s); });
	return zipN_iter<decltype(its), decltype(itends)>(its, itends);
    }

    auto end() {
	return zip_iter_end{};
    }

    auto begin() const {
	return const_cast<zipN_seq<Seqs...>*>(this)->begin();
    }

    auto end() const {
	return zip_iter_end{};
    }
};


// zip 2 sequence -> sequence of std::pair

template <typename Iter1, typename Iterend1, typename Iter2, typename Iterend2>
class zip2_iter {
private:
    Iter1	it1_;
    Iterend1	itend1_;
    Iter2	it2_;
    Iterend2	itend2_;

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::pair<decltype(*it1_), decltype(*it2_)>;
    using pointer		= value_type*;
    using reference		= value_type&;

    zip2_iter() = default;
    zip2_iter(const zip2_iter&) = default;
    zip2_iter(zip2_iter&&) = default;
    zip2_iter& operator=(const zip2_iter&) = default;
    zip2_iter& operator=(zip2_iter&&) = default;

    zip2_iter(Iter1 it1, Iterend1 itend1, Iter2 it2, Iterend2 itend2):
	it1_(it1), itend1_(itend1), it2_(it2), itend2_(itend2) {
    }

    bool operator==(const zip2_iter& o) const {
	return (it1_ == o.it1_ && it2_ == o.it2_);
    }

    bool operator!=(const zip2_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const zip_iter_end&) const {
	return (it1_ == itend1_ || it2_ == itend2_);
    }

    bool operator!=(const zip_iter_end& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	++it1_;
	++it2_;
	return *this;
    }

    decltype(auto) operator*() {
	return std::pair<decltype(*it1_), decltype(*it2_)>(*it1_, *it2_);
    }
};


template <typename Seq1, typename Seq2>
class zip2_seq {
private:
    using hold_type_1 =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq1>,
			       std::remove_reference_t<Seq1>,
			       Seq1>;
    using hold_type_2 =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq2>,
			       std::remove_reference_t<Seq2>,
			       Seq2>;
    hold_type_1 seq1_;
    hold_type_2 seq2_;

public:
    zip2_seq(Seq1 seq1, Seq2 seq2):
	seq1_(std::forward<Seq1>(seq1)), seq2_(std::forward<Seq2>(seq2)) {
    }

    zip2_seq(const zip2_seq&) = delete;
    zip2_seq& operator=(const zip2_seq&) = delete;
    zip2_seq(zip2_seq&&) = default;
    zip2_seq& operator=(zip2_seq&&) = delete;

    auto begin() {
	using std::begin;
	using std::end;
	auto it1	= begin(seq1_);
	auto itend1	= end(seq1_);
	auto it2	= begin(seq2_);
	auto itend2	= end(seq2_);
	return zip2_iter<decltype(it1), decltype(itend1),
			 decltype(it2), decltype(itend2)>(it1, itend1, it2, itend2);
    }

    auto end() {
	return zip_iter_end{};
    }

    auto begin() const {
	return const_cast<zip2_seq<Seq1, Seq2>*>(this)->begin();
    }

    auto end() const {
	return zip_iter_end{};
    }
};


template <typename... Seqs>
auto zip(Seqs&&... seqs)
{
    if constexpr (c7::typefunc::count<Seqs...>() == 2) {
	return zip2_seq<Seqs...>(std::forward<Seqs>(seqs)...);
    } else {
	return zipN_seq<Seqs...>(std::forward<Seqs>(seqs)...);
    }
}


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename... Seqs>
struct format_ident<c7::nseq::zip2_seq<Seqs...>> {
    static constexpr const char *name = "zip2";
};
template <typename... Seqs>
struct format_ident<c7::nseq::zipN_seq<Seqs...>> {
    static constexpr const char *name = "zipN";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/zip.hpp
