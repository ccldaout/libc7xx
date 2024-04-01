/*
 * c7nseq/string.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_STRING_HPP_LOADED_
#define C7_NSEQ_STRING_HPP_LOADED_


#include <cctype>
#include <string>
#include <c7nseq/_cmn.hpp>
#include <c7nseq/group.hpp>
#include <c7nseq/reverse.hpp>
#include <c7nseq/transform.hpp>
#include <c7nseq/vector.hpp>
#include <c7nseq/while.hpp>


namespace c7::nseq {


class to_string {
public:
    template <typename Seq>
    std::string operator()(const Seq& seq) {
	std::string s;
	for (auto c: seq) {
	    s += c;
	}
	return s;
    }
};


struct split_lines {
    template <typename Seq>
    auto operator()(Seq&& seq) {
	return std::forward<Seq>(seq)
	    | c7::nseq::group_by(
		[prev_ch=' '](auto h, auto c) mutable {
		    bool ret = (h != '\n' && prev_ch != '\n');
		    prev_ch = c;
		    return ret;
		})
	    | c7::nseq::transform(
		[](auto&& rng) {
		    return rng | c7::nseq::to_string();
		});
    }
};


struct trim {
    template <typename Seq>
    auto operator()(Seq&& seq) {
	// `reverse | filter | reverse' is don't work well.
	// so, to_string is needed before 2nd reverse.
	return std::forward<Seq>(seq)
	    | c7::nseq::reverse()
	    | c7::nseq::skip_while(
		[](auto c) {
		    return std::isspace(c);
		})
	    | c7::nseq::to_string()		// [IMPORTANT]
	    | c7::nseq::reverse()
	    | c7::nseq::skip_while(
		[](auto c) {
		    return std::isspace(c);
		});
    }
};


struct lower {
    template <typename Seq>
    auto operator()(Seq&& seq) {
	return std::forward<Seq>(seq)
	    | c7::nseq::transform(
		[](auto c) {
		    return std::tolower(c);
		});
    }
};


struct upper {
    template <typename Seq>
    auto operator()(Seq&& seq) {
	return std::forward<Seq>(seq)
	    | c7::nseq::transform(
		[](auto c) {
		    return std::toupper(c);
		});
    }
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <>
struct format_ident<std::string> {
    static constexpr const char *name = "str";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/string.hpp
