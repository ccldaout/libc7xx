/*
 * c7nseq/c_array.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ_C_ARRAY_HPP_LOADED__
#define C7_NSEQ_C_ARRAY_HPP_LOADED__


#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


template <typename T>
class c_array_obj {
private:    
    size_t n_;
    T *top_;

public:
    c_array_obj(size_t n, T *top): n_(n), top_(top) {}

    c_array_obj(const c_array_obj& o): n_(o.n_), top_(o.top_) {}

    c_array_obj(c_array_obj&& o): n_(o.n_), top_(o.top_) {
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


// c_array factory

template <int N, typename T>
auto c_array(T (&array)[N])
{
    return c_array_obj<T>(N, &array[0]);
}


template <typename T>
auto c_array(T* array, size_t n)
{
    return c_array_obj<T>(n, array);
}


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename T>
struct format_ident<c7::nseq::c_array_obj<T>> {
    static constexpr const char *name = "c_array";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/c_array.hpp
