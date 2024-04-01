/*
 * c7nseq/list.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_STORE_LIST_HPP_LOADED_
#define C7_NSEQ_STORE_LIST_HPP_LOADED_


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


class to_list {
public:
    template <typename Seq>
    auto operator()(Seq&& seq) {
	using std::begin;
	using item_type = typename std::iterator_traits<decltype(begin(seq))>::value_type;
	std::list<c7::typefunc::remove_cref_t<item_type>> vec;
	for (auto&& v: seq) {
	    vec.push_back(std::forward<decltype(v)>(v));
	}
	return vec;
    }
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <typename T>
struct format_ident<std::list<T>> {
    static constexpr const char *name = "list";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/list.hpp
