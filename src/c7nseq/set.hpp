/*
 * c7nseq/set.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_STORE_SET_HPP_LOADED__
#define C7_NSEQ_STORE_SET_HPP_LOADED__


#include <unordered_set>
#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


// new unordered_set

class to_set {
public:
    template <typename Seq>
    auto operator()(Seq&& seq) {
	using std::begin;
	using item_type = typename std::iterator_traits<decltype(begin(seq))>::value_type;
	std::unordered_set<c7::typefunc::remove_cref_t<item_type>> cnt;
	for (auto&& v: seq) {
	    cnt.insert(std::forward<decltype(v)>(v));
	}
	return cnt;
    }
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename T>
struct format_ident<std::unordered_set<T>> {
    static constexpr const char *name = "uset";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/set.hpp
