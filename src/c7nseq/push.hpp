/*
 * c7nseq/push.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_STORE_PUSH_HPP_LOADED__
#define C7_NSEQ_STORE_PUSH_HPP_LOADED__


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


// push_back

template <typename C>
class push_back_seq;

template <typename C>
class push_back_seq<C&> {
private:
    C& c_;

public:
    explicit push_back_seq(C& c): c_(c) {}

    push_back_seq(const push_back_seq&) = delete;
    push_back_seq& operator=(const push_back_seq&) = delete;
    push_back_seq(push_back_seq&&) = default;
    push_back_seq& operator=(push_back_seq&&) = delete;

    template <typename Seq>
    C& operator()(Seq&& seq) {
	for (auto&& v: seq) {
	    c_.push_back(std::forward<decltype(v)>(v));
	}
	return c_;
    }
};

template <typename C>
class push_back_seq<C&&> {
private:
    C c_;

public:
    explicit push_back_seq(C&& c): c_(std::move(c)) {}

    push_back_seq(const push_back_seq&) = delete;
    push_back_seq& operator=(const push_back_seq&) = delete;
    push_back_seq(push_back_seq&&) = default;
    push_back_seq& operator=(push_back_seq&&) = delete;

    template <typename Seq>
    C&& operator()(Seq&& seq) {
	for (auto&& v: seq) {
	    c_.push_back(std::forward<decltype(v)>(v));
	}
	return std::move(c_);
    }
};

template <typename C>
auto push_back(C&& c)
{
    return push_back_seq<decltype(c)>(std::forward<C>(c));
}


// push_front

template <typename C>
class push_front_seq;

template <typename C>
class push_front_seq<C&> {
private:
    C& c_;

public:
    explicit push_front_seq(C& c): c_(c) {}

    push_front_seq(const push_front_seq&) = delete;
    push_front_seq& operator=(const push_front_seq&) = delete;
    push_front_seq(push_front_seq&&) = default;
    push_front_seq& operator=(push_front_seq&&) = delete;

    template <typename Seq>
    C& operator()(Seq&& seq) {
	for (auto&& v: seq) {
	    c_.push_front(std::forward<decltype(v)>(v));
	}
	return c_;
    }
};

template <typename C>
class push_front_seq<C&&> {
private:
    C c_;

public:
    explicit push_front_seq(C&& c): c_(std::move(c)) {}

    push_front_seq(const push_front_seq&) = delete;
    push_front_seq& operator=(const push_front_seq&) = delete;
    push_front_seq(push_front_seq&&) = default;
    push_front_seq& operator=(push_front_seq&&) = delete;

    template <typename Seq>
    C&& operator()(Seq&& seq) {
	for (auto&& v: seq) {
	    c_.push_front(std::forward<decltype(v)>(v));
	}
	return std::move(c_);
    }
};

template <typename C>
auto push_front(C&& c)
{
    return push_front_seq<decltype(c)>(std::forward<C>(c));
}


} // namespace c7::nseq


#endif // c7nseq/push.hpp
