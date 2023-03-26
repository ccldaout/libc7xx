/*
 * c7nseq/insert.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ_STORE_INSERT_HPP_LOADED__
#define C7_NSEQ_STORE_INSERT_HPP_LOADED__


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


// insert

template <typename C>
class insert_obj;

template <typename C>
class insert_obj<C&> {
private:
    C& c_;

public:
    explicit insert_obj(C& c): c_(c) {}

    insert_obj(const insert_obj&) = delete;
    insert_obj& operator=(const insert_obj&) = delete;
    insert_obj(insert_obj&&) = default;
    insert_obj& operator=(insert_obj&&) = delete;

    template <typename Seq>
    C& operator()(Seq&& seq) {
	for (auto&& v: seq) {
	    c_.insert(std::forward<decltype(v)>(v));
	}
	return c_;
    }
};

template <typename C>
class insert_obj<C&&> {
private:
    C c_;

public:
    explicit insert_obj(C&& c): c_(std::move(c)) {}

    insert_obj(const insert_obj&) = delete;
    insert_obj& operator=(const insert_obj&) = delete;
    insert_obj(insert_obj&&) = default;
    insert_obj& operator=(insert_obj&&) = delete;

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
    return insert_obj<decltype(c)>(std::forward<C>(c));
}


// insert_or_assign

template <typename C>
class insert_or_assign_obj;

template <typename C>
class insert_or_assign_obj<C&> {
private:
    C& c_;

public:
    explicit insert_or_assign_obj(C& c): c_(c) {}

    insert_or_assign_obj(const insert_or_assign_obj&) = delete;
    insert_or_assign_obj& operator=(const insert_or_assign_obj&) = delete;
    insert_or_assign_obj(insert_or_assign_obj&&) = default;
    insert_or_assign_obj& operator=(insert_or_assign_obj&&) = delete;

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
class insert_or_assign_obj<C&&> {
private:
    C c_;

public:
    explicit insert_or_assign_obj(C&& c): c_(std::move(c)) {}

    insert_or_assign_obj(const insert_or_assign_obj&) = delete;
    insert_or_assign_obj& operator=(const insert_or_assign_obj&) = delete;
    insert_or_assign_obj(insert_or_assign_obj&&) = default;
    insert_or_assign_obj& operator=(insert_or_assign_obj&&) = delete;

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
    return insert_or_assign_obj<decltype(c)>(std::forward<C>(c));
}


} // namespace c7::nseq


#endif // c7nseq/insert.hpp
