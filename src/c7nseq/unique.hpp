/*
 * c7nseq/unique.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ_UNIQUE_HPP_LOADED__
#define C7_NSEQ_UNIQUE_HPP_LOADED__


#include <c7nseq/_cmn.hpp>
#include <c7nseq/filter.hpp>


namespace c7::nseq {


// factory

class unique {
public:
    template <typename Seq>
    auto operator()(Seq&& seq) {
	std::decay_t<decltype(*seq.begin())> save;
	bool next = false;
	auto pred = [save, next](const auto& v) mutable {
			if (next) {
			    if (save != v) {
				save = v;
				return true;
			    }
			    return false;
			} else {
			    next = true;
			    return true;
			}
		    };
	return filter(pred)(std::forward<Seq>(seq));
    }
};


} // namespace c7::nseq


#endif // c7nseq/unique.hpp
