/*
 * c7nseq/while.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_WHILE_HPP_LOADED__
#define C7_NSEQ_WHILE_HPP_LOADED__


#include <c7nseq/_cmn.hpp>
#include <c7nseq/filter.hpp>


namespace c7::nseq {


// factory

template <typename Predicate>
class take_while {
private:
    Predicate pred_;

public:
    explicit take_while(Predicate pred): pred_(pred) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	bool available = true;
	auto wrapper =
	    [available, pred=pred_](const auto& v) mutable {
		if (available) {
		    available = pred(v);
		}
		return available;
	    };
	return filter(wrapper)(std::forward<Seq>(seq));
    }
};


template <typename Predicate>
class skip_while {
private:
    Predicate pred_;

public:
    explicit skip_while(Predicate pred): pred_(pred) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	bool available = false;
	auto wrapper =
	    [available, pred=pred_](const auto& v) mutable {
		if (!available) {
		    available = !pred(v);
		}
		return available;
	    };
	return filter(wrapper)(std::forward<Seq>(seq));
    }
};


} // namespace c7::nseq


#endif // c7nseq/while.hpp
