/*
 * c7mpool.hpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __C7_MPOOL_HPP_LOADED__
#define __C7_MPOOL_HPP_LOADED__
#include "c7common.hpp"


#include <c7thread.hpp>
#include <memory>


namespace c7 {
namespace mpool {


/*----------------------------------------------------------------------------
                                  framework
----------------------------------------------------------------------------*/

template <typename T>
class mpool;


template <typename T>
struct item {
    union {
	mpool<T> *pool;
	item<T> *next;
    } link;
    T data;
};


template <typename T>
class pointer {
private:
    item<T> *s_;

public:
    pointer(const pointer&) = delete;
    pointer& operator=(const pointer&) = delete;

    pointer(): s_(nullptr) {}

    pointer(std::nullptr_t): s_(nullptr) {}

    explicit pointer(item<T> *s): s_(s) {}

    ~pointer() {
	if (s_ != nullptr) {
	    s_->link.pool->back(s_);
	    s_ = nullptr;
	}
    }

    pointer(pointer<T>&& o): s_(o.s_) {
	o.s_ = nullptr;
    }
	
    pointer<T>& operator=(pointer<T>&& o) {
	if (this != &o) {
	    if (s_ != nullptr) {
		s_->link.pool->back(s_);
	    }
	    s_ = o.s_;
	    o.s_ = nullptr;
	}
	return *this;
    }

    pointer<T>& operator=(std::nullptr_t) {
	if (s_ != nullptr) {
	    s_->link.pool->back(s_);
	    s_ = nullptr;
	}
	return *this;
    }

    operator bool() const {
	return s_ != nullptr;
    }

    bool operator==(const pointer<T>& o) {
	return s_ == o.s_;
    }

    bool operator==(nullptr_t) {
	return s_ == nullptr;
    }

    bool operator!=(const pointer<T>& o) {
	return s_ != o.s_;
    }

    bool operator!=(nullptr_t) {
	return s_ != nullptr;
    }

    inline T* operator->() const {
	return &s_->data;
    }

    inline T& operator*() const {
	return s_->data;
    }

    inline T* get() const {
	return s_ ? &s_->data : nullptr;
    }

    inline pointer<T> dup() {
	if (s_ == nullptr) {
	    return pointer<T>();
	}
	auto p = s_->link.pool->get();
	if (p) {
	    *p = s_->data;
	}
	return std::move(p);
    }
};


template <typename T>
class strategy {
public:
    strategy(const strategy<T>&) = delete;
    strategy<T>& operator=(const strategy<T>&) = delete;
    strategy(strategy<T>&&) = delete;
    strategy<T>& operator=(strategy<T>&&) = delete;
    strategy() {}
    virtual ~strategy() {};
    virtual c7::defer lock() = 0;
    virtual item<T> *alloc() = 0;		// lock() is already called by mpool
    virtual void freeall(item<T>* root) = 0;	// lock() is already called by mpool

    // next two functions are used under multithread, waitable strategy.
    virtual void wait() {}
    virtual void notify() {}
};


template <typename T>
class mpool {
private:
    std::unique_ptr<strategy<T>> strategy_;
    item<T> *free_;

public:
    typedef pointer<T> ptr;

    mpool(const mpool<T>&) = delete;
    mpool& operator=(const mpool<T>&) = delete;
    mpool(mpool<T>&&) = delete;
    mpool& operator=(mpool<T>&&) = delete;

    ~mpool() {
	auto defer = strategy_->lock();
	strategy_->freeall(free_);
	free_ = nullptr;
	defer();
    }

    mpool(std::unique_ptr<strategy<T>> strategy):
	strategy_(std::move(strategy)), free_(nullptr) {
    }

    pointer<T> get() {
	auto defer = strategy_->lock();
	item<T> *s;
	if (free_ == nullptr) {
	    s = strategy_->alloc();
	    if (s == nullptr) {
		while (free_ == nullptr) {
		    strategy_->wait();
		}
		s = free_;
	    }
	} else {
	    s = free_;
	}
	free_ = s->link.next;
	s->link.pool = this;
	return pointer<T>(s);
    }

    void back(item<T> *s) {
	auto defer = strategy_->lock();
	s->link.next = free_;
	free_ = s;
	strategy_->notify();
    }
};


/*----------------------------------------------------------------------------
                             allocation strategy
----------------------------------------------------------------------------*/

class dummy_lock {
public:
    c7::defer lock() {
	return c7::defer();
    }
};


// one by one allocation, no wait

template <typename T, typename Locker = c7::thread::spinlock>
class single_strategy: public strategy<T> {
private:
    Locker lock_;
    
public:
    single_strategy(): lock_() {}

    virtual ~single_strategy() {}

    virtual c7::defer lock() {
	return lock_.lock();
    }

    virtual item<T> *alloc() {
	auto s = new item<T>();
	s->link.next = nullptr;
	return s;
    }

    virtual void freeall(item<T>* root) {
	while (root != nullptr) {
	    auto next = root->link.next;
	    delete root;
	    root = next;
	}
    }
};


// multiple allocation, no wait

template <typename T, typename Locker = c7::thread::spinlock>
class chunk_strategy: public strategy<T> {
private:
    Locker lock_;
    int chunk_size_;			// item count by one chunk
    std::vector<item<T>*> chunks_;
    
public:
    chunk_strategy(int chunk_size): chunk_size_(chunk_size) {}

    virtual ~chunk_strategy() {
	freeall(nullptr);
    }

    virtual c7::defer lock() {
	return lock_.lock();
    }

    virtual item<T> *alloc() {
	auto s = new item<T>[chunk_size_];
	for (int i = 1; i < chunk_size_; i++) {
	    s[i-1].link.next = &s[i];
	}
	s[chunk_size_ - 1].link.next = nullptr;
	chunks_.push_back(s);
	return s;
    }

    virtual void freeall(item<T>*) {
	for (auto i: chunks_) {
	    delete i;
	}
	chunks_.clear();
    }
};


// limited multiple allocation, waitable

template <typename T>
class waitable_strategy: public strategy<T> {
private:
    c7::thread::condvar cv_;
    int chunk_size_;			// item count by one chunk
    int chunk_limit_;
    std::vector<item<T>*> chunks_;
    
public:
    waitable_strategy(int chunk_size, int chunk_limit):
	chunk_size_(chunk_size), chunk_limit_(chunk_limit) {
    }

    virtual ~waitable_strategy() {
	freeall(nullptr);
    }

    virtual c7::defer lock() {
	return cv_.lock();
    }

    virtual item<T> *alloc() {
	if (chunks_.size() == static_cast<size_t>(chunk_limit_)) {
	    return nullptr;
	}
	auto s = new item<T>[chunk_size_];
	for (int i = 1; i < chunk_size_; i++) {
	    s[i-1].link.next = &s[i];
	}
	s[chunk_size_ - 1].link.next = nullptr;
	chunks_.push_back(s);
	return s;
    }

    virtual void freeall(item<T>*) {
	for (auto i: chunks_) {
	    delete i;
	}
	chunks_.clear();
    }

    virtual void wait() {
	cv_.wait();
    }

    virtual void notify() {
	cv_.notify_all();
    }
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace mpool


/*----------------------------------------------------------------------------
                            convenience definition
----------------------------------------------------------------------------*/

template <typename T>
using mpool_ptr = c7::mpool::pointer<T>;


template <typename T, typename Locker = c7::thread::spinlock>
mpool::mpool<T> mpool_1by1()
{
    using strategy_t = mpool::strategy<T>;
    auto s = new mpool::single_strategy<T, Locker>();
    auto bs = static_cast<strategy_t*>(s);
    return mpool::mpool<T>(std::unique_ptr<strategy_t>(bs));
}


template <typename T, typename Locker = c7::thread::spinlock>
mpool::mpool<T> mpool_chunk(int chunk_size)
{
    using strategy_t = mpool::strategy<T>;
    auto s = new mpool::chunk_strategy<T, Locker>(chunk_size);
    auto bs = static_cast<strategy_t*>(s);
    return mpool::mpool<T>(std::unique_ptr<strategy_t>(bs));
}


template <typename T>
mpool::mpool<T> mpool_waitable(int chunk_size, int chunk_limit)
{
    using strategy_t = mpool::strategy<T>;
    auto s = new mpool::waitable_strategy<T>(chunk_size, chunk_limit);
    auto bs = static_cast<strategy_t*>(s);
    return mpool::mpool<T>(std::unique_ptr<strategy_t>(bs));
}


} // namespace c7


#endif // c7mpool.hpp
