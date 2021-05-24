/*
 * c7mm.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1872940208
 */
#ifndef C7_MM_HPP_LOADED__
#define C7_MM_HPP_LOADED__
#include <c7common.hpp>


#include <c7fd.hpp>
#include <c7utils.hpp>
#include <unistd.h>
#include <iterator>


namespace c7 {


/*----------------------------------------------------------------------------
                            anonymous mmap manager
----------------------------------------------------------------------------*/

class anon_mmap_manager {
public:
    static constexpr size_t PAGESIZE = 8192;

    anon_mmap_manager(const anon_mmap_manager&) = delete;
    anon_mmap_manager& operator=(const anon_mmap_manager&) = delete;

    anon_mmap_manager() {}
    anon_mmap_manager(anon_mmap_manager&& o);
    anon_mmap_manager& operator=(anon_mmap_manager&& o);

    ~anon_mmap_manager() {
	reset();
    }

    c7::result<std::pair<void *, size_t>> reserve(size_t size);
    void reset();

private:
    size_t map_size_ = 0;
    void *map_addr_ = nullptr;
};


/*----------------------------------------------------------------------------
                            switchable mmap object
----------------------------------------------------------------------------*/

class mmobj {
public:
    mmobj(const mmobj&) = delete;
    mmobj& operator=(const mmobj&) = delete;
    
    mmobj(): top_(nullptr), size_(0), threshold_(0), path_(), fd_() {}
    ~mmobj();

    mmobj(mmobj&& o);
    mmobj& operator=(mmobj&& o);

    result<> init(size_t size, size_t threshold = -1UL, const std::string& path = "");
    result<> resize(size_t new_size);
    void reset();
    std::pair<void*, size_t> operator()();

private:
    static constexpr size_t PAGESIZE = 8192;

    void *top_;
    size_t size_;
    size_t threshold_;
    std::string path_;
    c7::fd fd_;

    result<> resize_anon_mm(size_t size);
    result<> switch_to_file_mm(size_t size);
    result<> resize_file_mm(size_t size);
};


/*----------------------------------------------------------------------------
                             mmobj baseed vector
----------------------------------------------------------------------------*/

template <typename T>
class mmvec {
public:
    typedef c_array_iterator<T> iterator;
    typedef c_array_iterator<const T> const_iterator;

    mmvec(const mmvec&) = delete;
    mmvec& operator=(const mmvec&) = delete;

    mmvec(): top_(nullptr), capa_(), n_(), mm_() {}

    mmvec(mmvec&& o) {
	top_  = o.top_;
	capa_ = o.capa_;
	n_    = o.n_;
	mm_   = std::move(o.mm_);
	o.top_  = nullptr;
	o.capa_ = 0;
	o.n_    = 0;
    }

    mmvec& operator=(mmvec&& o) {
	if (this != &o) {
	    top_  = o.top_;
	    capa_ = o.capa_;
	    n_    = o.n_;
	    mm_   = std::move(o.mm_);
	    o.top_  = nullptr;
	    o.capa_ = 0;
	    o.n_    = 0;
	}
	return *this;
    }

    result<> init(size_t size, size_t threshold, const std::string& path) {
	if (auto res = mm_.init(size, threshold, path); !res) {
	    return res;
	}
	auto [p, z] = mm_();
	top_  = static_cast<T*>(p);
	capa_ = z / sizeof(T);
	return c7result_ok();
    }	

    void reset() {
	*this = std::move(mmvec<T>());
    }

    result<> extend() {
	auto req = capa_ * 2 * sizeof(T);
	if (auto res = mm_.resize(req); !res) {
	    return res;
	}
	auto [p, z] = mm_();
	top_  = static_cast<T*>(p);
	capa_ = z / sizeof(T);
	return c7result_ok();
    }

    void extend_exc() {
	if (auto res = extend(); !res) {
	    throw std::runtime_error(c7::format("%{}", res));
	}
    }

    void push_back(const T& item) {
	if (n_ == capa_) {
	    extend_exc();
	}
	top_[n_++] = item;
    }

    void push_back(T&& item) {
	if (n_ == capa_) {
	    extend_exc();
	}
	top_[n_++] = std::move(item);
    }

    result<> append(const T& item) {
	if (n_ == capa_) {
	    if (auto res = extend(); !res) {
		return res;
	    }
	}
	top_[n_++] = item;
	return c7result_ok();
    }

    result<> append(T&& item) {
	if (n_ == capa_) {
	    if (auto res = extend(); !res) {
		return res;
	    }
	}
	top_[n_++] = std::move(item);
	return c7result_ok();
    }

    void pop_back() {
	if (n_ > 0) {
	    n_--;
	}
    }

    void clear() {
	n_ = 0;
    }

    bool empty() const {
	return n_ == 0;
    }

    size_t size() const {
	return n_;
    }

    T* data() {
	return top_;
    }

    const T* data() const {
	return top_;
    }

    T& operator[](size_t n) {
	return top_[n];
    }

    const T& operator[](size_t n) const {
	return top_[n];
    }

    iterator begin() {
	return iterator(top_, 0);
    }

    iterator end() {
	return iterator(top_, n_);
    }

    const_iterator begin() const {
	return const_iterator(const_cast<const T*>(top_), 0);
    }

    const_iterator end() const {
	return const_iterator(const_cast<const T*>(top_), n_);
    }

    auto rbegin() {
	return std::reverse_iterator<iterator>(end());
    }

    auto rend() {
	return std::reverse_iterator<iterator>(begin());
    }

    auto rbegin() const {
	return std::reverse_iterator<const_iterator>(
	    const_iterator(const_cast<const T*>(top_), n_));
    }

    auto rend() const {
	return std::reverse_iterator<const_iterator>(
	    const_iterator(const_cast<const T*>(top_), 0));
    }

private:
    T *top_;
    size_t capa_;
    size_t n_;
    mmobj mm_;
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7mm.hpp
