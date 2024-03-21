/*
 * c7nseq/c_array.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_C_ARRAY_HPP_LOADED__
#define C7_NSEQ_C_ARRAY_HPP_LOADED__


#include <memory>
#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


template <typename T>
class c_array_seq {
private:
    size_t n_;
    T *top_;

public:
    c_array_seq(size_t n, T *top): n_(n), top_(top) {}

    c_array_seq(const c_array_seq& o): n_(o.n_), top_(o.top_) {}

    c_array_seq(c_array_seq&& o): n_(o.n_), top_(o.top_) {
	o.top_ = nullptr;
	o.n_ = 0;
    }

    auto size() const {
	return n_;
    }

    auto begin() {
	return top_;
    }

    auto end() {
	return top_ + n_;
    }

    auto begin() const {
	return top_;
    }

    auto end() const {
	return top_ + n_;
    }

    auto rbegin() {
	return top_ + n_ - 1;
    }

    auto rend() {
	return top_ - 1;
    }

    auto rbegin() const {
	return top_ + n_ - 1;
    }

    auto rend() const {
	return top_ - 1;
    }
};


template <typename T>
class c_array_seq<std::shared_ptr<T>> {
private:
    size_t n_;
    std::shared_ptr<T> top_;

public:
    c_array_seq(size_t n, std::shared_ptr<T> top): n_(n), top_(std::move(top)) {}

    c_array_seq(const c_array_seq& o): n_(o.n_), top_(o.top_) {}

    c_array_seq(c_array_seq&& o): n_(o.n_), top_(std::move(o.top_)) {
	o.top_ = nullptr;
	o.n_ = 0;
    }

    auto size() const {
	return n_;
    }

    auto begin() {
	return top_.get();
    }

    auto end() {
	return top_.get() + n_;
    }

    auto begin() const {
	return top_.get();
    }

    auto end() const {
	return top_.get() + n_;
    }

    auto rbegin() {
	return top_.get() + n_ - 1;
    }

    auto rend() {
	return top_.get() - 1;
    }

    auto rbegin() const {
	return top_.get() + n_ - 1;
    }

    auto rend() const {
	return top_.get() - 1;
    }
};


template <typename T, typename D>
class c_array_seq<std::unique_ptr<T, D>> {
private:
    size_t n_;
    std::unique_ptr<T, D> top_;

public:
    c_array_seq(size_t n, std::unique_ptr<T, D> top): n_(n), top_(std::move(top)) {}

    c_array_seq(c_array_seq&& o): n_(o.n_), top_(std::move(o.top_)) {
	o.n_ = 0;
    }

    auto size() const {
	return n_;
    }

    auto begin() {
	return top_.get();
    }

    auto end() {
	return top_.get() + n_;
    }

    auto begin() const {
	return top_.get();
    }

    auto end() const {
	return top_.get() + n_;
    }

    auto rbegin() {
	return top_.get() + n_ - 1;
    }

    auto rend() {
	return top_.get() - 1;
    }

    auto rbegin() const {
	return top_.get() + n_ - 1;
    }

    auto rend() const {
	return top_.get() - 1;
    }
};


// c_array factory

template <int N, typename T>
auto c_array(T (&array)[N])
{
    return c_array_seq<T>(N, &array[0]);
}


template <typename T>
auto c_array(T* array, size_t n)
{
    return c_array_seq<T>(n, array);
}


template <typename T>
auto c_array(std::shared_ptr<T> array, size_t n)
{
    return c_array_seq<std::shared_ptr<T>>(n, std::move(array));
}


template <typename T, typename D = std::default_delete<T>>
auto c_array(std::unique_ptr<T, D> array, size_t n)
{
    return c_array_seq<std::unique_ptr<T, D>>(n, std::move(array));
}


template <typename T, typename U,
	  typename = std::enable_if_t<std::is_pointer_v<U> ||
				      std::is_same_v<U, std::nullptr_t>>>
auto c_array(T* array, U term) {
    size_t n;
    for (n = 0; array[n] != term; n++);
    return c_array(array, n);
}


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename T>
struct format_ident<c7::nseq::c_array_seq<T>> {
    static constexpr const char *name = "c_array";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/c_array.hpp
