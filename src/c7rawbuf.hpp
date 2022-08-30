/*
 * c7rawbuf.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=566388067
 */
#ifndef C7_RAWBUF_HPP_LOADED__
#define C7_RAWBUF_HPP_LOADED__
#include <c7common.hpp>


#include <cstdlib>
#include <cstring>
#include <c7fd.hpp>
#include <c7format.hpp>
#include <c7utils.hpp>


namespace c7 {


class std_memory_manager {
public:
    std_memory_manager(const std_memory_manager&) = delete;
    std_memory_manager& operator=(const std_memory_manager&) = delete;

    std_memory_manager() {}
    std_memory_manager(std_memory_manager&& o);
    std_memory_manager& operator=(std_memory_manager&& o);

    ~std_memory_manager() {
	reset();
    }

    c7::result<std::pair<void *, size_t>> reserve(size_t size);
    void reset();

private:
    size_t size_ = 0;
    void *addr_ = nullptr;
};


// MM: std_memory_manager/anon_mmap_manager
template <typename T, typename MM = std_memory_manager>
class rawbuf {
public:
    typedef c_array_iterator<T> iterator;
    typedef c_array_iterator<const T> const_iterator;

    rawbuf(const rawbuf&) = delete;
    rawbuf& operator=(const rawbuf&) = delete;

    rawbuf() {};
    rawbuf(rawbuf&& o);
    rawbuf& operator=(rawbuf&& o);

    ~rawbuf() {
	mm_.reset();
    }

    c7::result<> reserve(size_t n_req);

    c7::result<> push_back(const T& d) {
	if (n_rsv_ == n_cur_) {
	    if (auto res = extend(); !res) {
		return res;
	    }
	}
	top_[n_cur_++] = d;
	return c7result_ok();
    }

    c7::result<> append_from(c7::fd& fd);

    c7::result<> append_to(c7::fd& fd);

    c7::result<> flush_to(c7::fd& fd);

    T *data() {
	return top_;
    }

    const T *data() const {
	return top_;
    }

    void *void_p(size_t n_elm = 0) {
	return static_cast<void *>(top_ + n_elm);
    }

    const void *void_p(size_t n_elm = 0) const {
	return static_cast<void *>(top_ + n_elm);
    }

    size_t size() const {
	return n_cur_;
    }

    size_t nbytes() const {
	return n_cur_ * sizeof(T);
    }

    void clear() {
	n_cur_ = 0;
    }

    void reset();

    void free() {
	reset();
    }

    double coef() const {
	return coef_;
    }

    void set_coef(double coef);

    T& operator[](size_t n) {
	return top_[n];
    }

    const T& operator[](size_t n) const {
	return top_[n];
    }

    auto begin() {
	return iterator(top_, 0);
    }

    auto end() {
	return iterator(top_, n_cur_);
    }

    auto begin() const {
	return const_iterator(const_cast<const T*>(top_), 0);
    }

    auto end() const {
	return const_iterator(const_cast<const T*>(top_), n_cur_);
    }

    auto rbegin() {
	return std::reverse_iterator<iterator>(end());
    }

    auto rend() {
	return std::reverse_iterator<iterator>(begin());
    }

    auto rbegin() const {
	return std::reverse_iterator<const_iterator>(
	    const_iterator(const_cast<const T*>(top_, n_cur_)));
    }

    auto rend() const {
	return std::reverse_iterator<const_iterator>(
	    const_iterator(const_cast<const T*>(top_, 0)));
    }

private:
    MM mm_;
    T *top_ = nullptr;
    size_t n_rsv_ = 0;
    size_t n_cur_ = 0;
    double coef_ = 1.6;

    c7::result<> extend();
};


template <typename T, typename MM>
rawbuf<T, MM>::rawbuf(rawbuf&& o):
    mm_(std::move(o.mm_)), top_(o.top_), n_rsv_(o.n_rsv_), n_cur_(o.n_cur_), coef_(o.coef_)
{
    o.top_ = nullptr;
    o.n_rsv_ = o.n_cur_ = 0;
}


template <typename T, typename MM>
auto
rawbuf<T, MM>::operator=(rawbuf&& o) -> rawbuf&
{
    if (this != &o) {
	mm_    = std::move(o.mm_);
	top_   = o.top_;
	n_rsv_ = o.n_rsv_;
	n_cur_ = o.n_cur_;
	coef_  = o.coef_;
	o.top_ = nullptr;
	o.n_rsv_ = o.n_cur_ = 0;
    }
    return *this;
}


template <typename T, typename MM>
c7::result<>
rawbuf<T, MM>::reserve(size_t n_req)
{
    if (n_req > n_rsv_) {
	auto res = mm_.reserve(n_req * sizeof(T));
	if (!res) {
	    return res;
	}
	auto [addr, size] = std::move(res.value());
	top_ = static_cast<T*>(addr);
	n_rsv_ = size / sizeof(T);
    }
    return c7result_ok();
}


template <typename T, typename MM>
c7::result<>
rawbuf<T, MM>::extend()
{
    size_t constexpr init_req = std::max<size_t>(64, 8192 / sizeof(T));
    size_t n_req;
    if (n_rsv_ == 0) {
	n_req = init_req;
    } else {
	n_req = n_rsv_ * coef_;
	if (n_req == n_rsv_) {
	    n_req += init_req;
	}
    }
    if (auto res = reserve(n_req); !res) {
	return res;
    }
    return c7result_ok();
}


template <typename T, typename MM>
c7::result<>
rawbuf<T, MM>::append_from(c7::fd& fd)
{
    size_t n_read;
    if (auto res = fd.stat(); !res) {
	return res;
    } else {
	n_read = res.value().st_size / sizeof(T);
    }
    size_t n_req = n_cur_ + n_read;
    if (auto res = reserve(n_req); !res) {
	return res;
    }
    if (auto res = fd.read_n(void_p(n_cur_), n_read * sizeof(T)); !res) {
	return std::move(res.get_result());
    }
    n_cur_ += n_read;
    return c7result_ok();
}


template <typename T, typename MM>
c7::result<>
rawbuf<T, MM>::append_to(c7::fd& fd)
{
    if (auto res = fd.write_n(void_p(), nbytes()); !res) {
	return std::move(res.get_result());
    }
    return c7result_ok();
}


template <typename T, typename MM>
c7::result<>
rawbuf<T, MM>::flush_to(c7::fd& fd)
{
    if (auto res = append_to(fd); !res) {
	return res;
    }
    clear();
    return c7result_ok();
}


template <typename T, typename MM>
void
rawbuf<T, MM>::reset()
{
    mm_.reset();
    top_ = nullptr;
    n_rsv_ = 0;
    n_cur_ = 0;
}


template <typename T, typename MM>
void
rawbuf<T, MM>::set_coef(double coef)
{
    if (coef > 1.0) {
	coef_ = coef;
    }
}


} // namespace c7


#endif // c7rawbuf.hpp
