/*
 * c7nseq/insert.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_STORE_INSERT_HPP_LOADED_
#define C7_NSEQ_STORE_INSERT_HPP_LOADED_


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


// insert

template <typename C>
class insert_seq;

template <typename C>
class insert_seq<C&> {
private:
    C& c_;

public:
    explicit insert_seq(C& c): c_(c) {}

    insert_seq(const insert_seq&) = delete;
    insert_seq& operator=(const insert_seq&) = delete;
    insert_seq(insert_seq&&) = default;
    insert_seq& operator=(insert_seq&&) = delete;

    template <typename Seq>
    C& operator()(Seq&& seq) {
	for (auto&& v: seq) {
	    c_.insert(std::forward<decltype(v)>(v));
	}
	return c_;
    }
};

template <typename C>
class insert_seq<C&&> {
private:
    C c_;

public:
    explicit insert_seq(C&& c): c_(std::move(c)) {}

    insert_seq(const insert_seq&) = delete;
    insert_seq& operator=(const insert_seq&) = delete;
    insert_seq(insert_seq&&) = default;
    insert_seq& operator=(insert_seq&&) = delete;

    template <typename Seq>
    C&& operator()(Seq&& seq) {
	for (auto&& v: seq) {
	    c_.insert(std::forward<decltype(v)>(v));
	}
	return std::move(c_);
    }
};

template <typename C>
auto insert(C&& c)
{
    return insert_seq<decltype(c)>(std::forward<C>(c));
}


// insert_or_assign

template <typename C>
class insert_or_assign_seq;

template <typename C>
class insert_or_assign_seq<C&> {
private:
    C& c_;

public:
    explicit insert_or_assign_seq(C& c): c_(c) {}

    insert_or_assign_seq(const insert_or_assign_seq&) = delete;
    insert_or_assign_seq& operator=(const insert_or_assign_seq&) = delete;
    insert_or_assign_seq(insert_or_assign_seq&&) = default;
    insert_or_assign_seq& operator=(insert_or_assign_seq&&) = delete;

    template <typename Seq>
    C& operator()(Seq&& seq) {
	for (auto&& v: seq) {
	    c_.insert_or_assign(std::forward<decltype(v.first)>(v.first),
				std::forward<decltype(v.second)>(v.second));
	}
	return c_;
    }
};

template <typename C>
class insert_or_assign_seq<C&&> {
private:
    C c_;

public:
    explicit insert_or_assign_seq(C&& c): c_(std::move(c)) {}

    insert_or_assign_seq(const insert_or_assign_seq&) = delete;
    insert_or_assign_seq& operator=(const insert_or_assign_seq&) = delete;
    insert_or_assign_seq(insert_or_assign_seq&&) = default;
    insert_or_assign_seq& operator=(insert_or_assign_seq&&) = delete;

    template <typename Seq>
    C&& operator()(Seq&& seq) {
	for (auto&& v: seq) {
	    c_.insert_or_assign(std::forward<decltype(v.first)>(v.first),
				std::forward<decltype(v.second)>(v.second));
	}
	return std::move(c_);
    }
};

template <typename C>
auto insert_or_assign(C&& c)
{
    return insert_or_assign_seq<decltype(c)>(std::forward<C>(c));
}


} // namespace c7::nseq


#endif // c7nseq/insert.hpp
