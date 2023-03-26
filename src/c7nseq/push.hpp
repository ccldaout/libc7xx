/*
 * c7nseq/push.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ_STORE_PUSH_HPP_LOADED__
#define C7_NSEQ_STORE_PUSH_HPP_LOADED__


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


// push_back

template <typename C>
class push_back_obj;

template <typename C>
class push_back_obj<C&> {
private:
    C& c_;

public:
    explicit push_back_obj(C& c): c_(c) {}

    push_back_obj(const push_back_obj&) = delete;
    push_back_obj& operator=(const push_back_obj&) = delete;
    push_back_obj(push_back_obj&&) = default;
    push_back_obj& operator=(push_back_obj&&) = delete;

    template <typename Seq>
    C& operator()(Seq&& seq) {
	for (auto&& v: seq) {
	    c_.push_back(std::forward<decltype(v)>(v));
	}
	return c_;
    }
};

template <typename C>
class push_back_obj<C&&> {
private:
    C c_;

public:
    explicit push_back_obj(C&& c): c_(std::move(c)) {}

    push_back_obj(const push_back_obj&) = delete;
    push_back_obj& operator=(const push_back_obj&) = delete;
    push_back_obj(push_back_obj&&) = default;
    push_back_obj& operator=(push_back_obj&&) = delete;

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
    return push_back_obj<decltype(c)>(std::forward<C>(c));
}


// push_front

template <typename C>
class push_front_obj;

template <typename C>
class push_front_obj<C&> {
private:
    C& c_;

public:
    explicit push_front_obj(C& c): c_(c) {}

    push_front_obj(const push_front_obj&) = delete;
    push_front_obj& operator=(const push_front_obj&) = delete;
    push_front_obj(push_front_obj&&) = default;
    push_front_obj& operator=(push_front_obj&&) = delete;

    template <typename Seq>
    C& operator()(Seq&& seq) {
	for (auto&& v: seq) {
	    c_.push_front(std::forward<decltype(v)>(v));
	}
	return c_;
    }
};

template <typename C>
class push_front_obj<C&&> {
private:
    C c_;

public:
    explicit push_front_obj(C&& c): c_(std::move(c)) {}

    push_front_obj(const push_front_obj&) = delete;
    push_front_obj& operator=(const push_front_obj&) = delete;
    push_front_obj(push_front_obj&&) = default;
    push_front_obj& operator=(push_front_obj&&) = delete;

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
    return push_front_obj<decltype(c)>(std::forward<C>(c));
}


} // namespace c7::nseq


#endif // c7nseq/push.hpp
