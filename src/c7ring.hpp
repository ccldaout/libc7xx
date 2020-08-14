/*
 * c7ring.hpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __C7_RING_HPP_LOADED__
#define __C7_RING_HPP_LOADED__
#include <c7common.hpp>


#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <type_traits>


namespace c7 {


/*----------------------------------------------------------------------------
                           ring: cyclic linked-list
----------------------------------------------------------------------------*/

template <typename T>
class ring_iterator {
private:
    T* ref_;
    T* cur_;
    bool terminator_;
    bool reverse_;

    void forward() {
	if (cur_ != nullptr) {
	    cur_ = &(*cur_).next();
	    if (cur_ == ref_) {
		cur_ = nullptr;
	    }
	} else if (reverse_) {
	    cur_ = &(*ref_).next();
	}
    }

    void backward() {
	if (cur_ != nullptr) {
	    cur_ = &(*cur_).prev();
	    if (cur_ == ref_) {
		cur_ = nullptr;
	    }
	} else if (!reverse_) {
	    cur_ = &(*ref_).prev();
	}
    }

public:
    typedef ptrdiff_t difference_type;
    typedef T value_type;
    typedef value_type* pointer;
    typedef value_type& reference;
    typedef std::input_iterator_tag iterator_category;

    ring_iterator(T* ref, bool terminator, bool reverse):
	ref_(ref), cur_(ref), terminator_(terminator), reverse_(reverse) {}
	
    bool operator==(const ring_iterator<T>& o) {
	return this->cur_ == o.cur_;
    }

    bool operator!=(const ring_iterator<T>& o) {
	return !(*this == o);
    }

    T& operator*() {
	return *cur_;
    }

    ring_iterator& operator++() {
	if (!terminator_) {
	    if (reverse_)
		backward();
	    else
		forward();
	}
	return *this;
    }

    ring_iterator& operator--() {
	if (!terminator_) {
	    if (reverse_)
		forward();
	    else
		backward();
	}
	return *this;
    }
};


template <typename T>
struct ring_traits {
    static inline T* member_access(T& data) {
	return &data;
    }
    static inline T& deref(T& data) {
	return data;
    }
};

template <typename T>
struct ring_traits<T&> {
    static inline T* member_access(T& data) {
	return &data;
    }
    static inline T& deref(T& data) {
	return data;
    }
};

template <typename T>
struct ring_traits<T*> {
    static inline T* member_access(T* data) {
	return data;
    }
    static inline T& deref(T* data) {
	return *data;
    }
};

template <typename T>
struct ring_traits<std::unique_ptr<T>> {
    typedef std::unique_ptr<T> type;
    static inline T* member_access(type& data) {
	return data.get();
    }
    static inline T& deref(type& data) {
	return *data;
    }
};

template <typename T>
struct ring_traits<std::shared_ptr<T>> {
    typedef std::shared_ptr<T> type;
    static inline T* member_access(type& data) {
	return data.get();
    }
    static inline T& deref(type& data) {
	return *data;
    }
};


template <typename T>
class ring {
private:    
    ring<T>* prev_;
    ring<T>* next_;
    T data_;

public:
    typedef ring_iterator<ring<T>> iterator;

public:
    ring(): prev_(this), next_(this), data_() {}

    // copy/move constructor and copy/move assignment are all disabled,
    // because other ring may have a pointer to the ring passed by argument as 'o'
    // in the following code. And it mean that when the ring passed by 'o' is deleted,
    // the other rings have dangling pointer.
    ring(const ring<T>& o) = delete;
    ring(ring<T>&& o) = delete;
    ring<T>& operator=(const ring<T>& o) = delete;
    ring<T>& operator=(ring<T>&& o) = delete;

    template <typename U>
    explicit ring(const U& data): prev_(this), next_(this), data_(data) {}

    template <typename U>
    explicit ring(U&& data): prev_(this), next_(this), data_(std::forward<T>(data)) {}

    template <typename U>
    ring<T>& operator=(const U& data) {
	data_ = data;
	return *this;
    }

    template <typename U>
    ring<T>& operator=(U&& data) {
	data_ = std::forward<T>(data);
	return *this;
    }

    inline auto operator->() {
	return ring_traits<T>::member_access(data_);
    }

    inline auto operator*() {
	return ring_traits<T>::deref(data_);
    }

    inline ring<T>& next() {
	return *next_;
    }

    inline ring<T>& prev() {
	return *prev_;
    }

    // before: this_prev -> this && other_prev -> other -> ..o..
    // after : this_prev -> other -> ..o.. -> other_prev -> this
    ring<T>& insert_prev(ring<T>& o) {
	auto this_prev  = prev_;
	auto other      = &o;
	auto other_prev = o.prev_;
	this->prev_       = other_prev;
	this_prev->next_  = other;
	other->prev_      = this_prev;
	other_prev->next_ = this;
	return *this;
    }

    // before: this -> this_next && other_prev -> other -> ..o..
    // after : this -> other -> ..o.. -> other_prev -> this_next
    ring<T>& insert_next(ring<T>& o) {
	auto this_next  = next_;
	auto other      = &o;
	auto other_prev = o.prev_;
	this->next_       = other;
	this_next->prev_  = other_prev;
	other->prev_      = this;
	other_prev->next_ = this_next;
	return *this;
    }

    ring<T>& unlink() {
	auto this_prev = prev_;
	auto this_next = next_;
	this_prev->next_ = this_next;
	this_next->prev_ = this_prev;
	this->prev_ = this->next_ = this;
	return *this;
    }

    ring<T>& unlink(ring<T>& edge) {
	auto this_prev = prev_;
	auto edge_next = edge.next_;
	this_prev->next_ = edge_next;
	edge_next->prev_ = this_prev;
	this->prev_ = &edge;
	edge.next_  = this;
	return *this;
    }

    inline T& ref() {
	return data_;
    }

    inline std::remove_reference_t<T> move() {
	return std::move(data_);
    }

    iterator begin() {
	return iterator(this, false, false);
    }

    iterator end() {
	return iterator(nullptr, true, false);
    }

    iterator rbegin() {
	return iterator(this->prev_, false, true);
    }

    iterator rend() {
	return iterator(nullptr, true, true);
    }
};


template <typename T>
ring<T> make_ring(const T& data)
{
    return ring<T>(data);
}

template <typename T>
ring<T> make_ring(T&& data)
{
    return ring<T>(std::forward<T>(data));
}

template <typename T>
ring<T>* new_ring(const T& data)
{
    return new ring<T>(data);
}

template <typename T>
ring<T>* new_ring(T&& data)
{
    return new ring<T>(std::forward<T>(data));
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7ring.hpp
