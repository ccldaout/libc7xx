/*
 * c7nseq/map.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_STORE_MAP_HPP_LOADED__
#define C7_NSEQ_STORE_MAP_HPP_LOADED__


#include <unordered_map>
#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


// new unordered_map

class to_map {
public:
    template <typename Seq>
    auto operator()(Seq&& seq) {
	using std::begin;
	using key_type = typename c7::typefunc::remove_cref_t<decltype(*begin(seq))>::first_type;
	using val_type = typename c7::typefunc::remove_cref_t<decltype(*begin(seq))>::second_type;
	std::unordered_map<c7::typefunc::remove_cref_t<key_type>,
			   c7::typefunc::remove_cref_t<val_type>> cnt;
	for (auto&& v: seq) {
	    cnt.insert_or_assign(std::forward<decltype(v.first)>(v.first),
				 std::forward<decltype(v.second)>(v.second));
	}
	return cnt;
    }
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename K, typename V>
struct format_ident<std::unordered_map<K, V>> {
    static constexpr const char *name = "umap";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/map.hpp
