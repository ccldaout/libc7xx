/*
 * c7mpool.hpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_MPOOL_HPP_LOADED__
#define C7_MPOOL_HPP_LOADED__
#include <c7common.hpp>


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
class strategy;


template <typename T>
struct item {
    union {
	strategy<T> *pool;
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

private:
    friend class mpool<T>;
    friend class pointer<T>;

    item<T> *free_ = nullptr;
    bool mpool_attached_ = false;
    size_t lending_ = 0;

    pointer<T> get() {
	auto defer = lock();
	lending_++;
	item<T> *s;
	if (free_ == nullptr) {
	    s = alloc();
	    if (s == nullptr) {
		while (free_ == nullptr) {
		    wait();
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
	auto defer = lock();
	s->link.next = free_;
	free_ = s;
	lending_--;
	notify();
	if (lending_ == 0 && !mpool_attached_) {
	    defer();
	    delete this;
	}
    }

    void attached() {
	auto defer = lock();
	mpool_attached_ = true;
    }

    void detached() {
	auto defer = lock();
	mpool_attached_ = false;
	if (lending_ == 0 && !mpool_attached_) {
	    defer();
	    delete this;
	}
    }

protected:
    virtual ~strategy() {}
    virtual c7::defer lock() = 0;
    virtual item<T> *alloc() = 0;
    virtual void wait() {}
    virtual void notify() {}
};


template <typename T>
class mpool {
private:
    strategy<T> *strategy_ = nullptr;

public:
    mpool(mpool<T>&& o): strategy_(o.strategy_) {
	o.strategy_ = nullptr;
    }

    mpool& operator=(mpool<T>&& o) {
	if (this != &o) {
	    this->strategy_ = o.strategy_;
	    o.strategy_ = nullptr;
	}
	return *this;
    }

    mpool(): strategy_(nullptr) {}

    ~mpool() {
	if (strategy_ != nullptr) {
	    strategy_->detached();
	}
    }

    
    template <typename... Args>
    mpool(strategy<T>*(*maker)(Args...), Args... args) {
	strategy_ = maker(args...);
	strategy_->attached();
    }

    template <typename... Args>
    void set_strategy(strategy<T>*(*maker)(Args...), Args... args) {
	if (strategy_ != nullptr) {
	    strategy_->detached();
	}
	strategy_ = maker(args...);
	strategy_->attached();
    }

    pointer<T> get() {
	return strategy_->get();
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


// multiple allocation, no wait

template <typename T, typename Locker = c7::thread::spinlock>
class chunk_strategy: public strategy<T> {
private:
    Locker lock_;
    int chunk_size_;			// item count by one chunk
    std::vector<item<T>*> chunks_;

    chunk_strategy(int chunk_size): chunk_size_(chunk_size) {}

    ~chunk_strategy() {
	freeall();
    }

    void freeall() {
	for (auto i: chunks_) {
	    delete[] i;
	}
	chunks_.clear();
    }

    c7::defer lock() {
	return lock_.lock();
    }

    item<T> *alloc() {
	auto s = new item<T>[chunk_size_];
	for (int i = 1; i < chunk_size_; i++) {
	    s[i-1].link.next = &s[i];
	}
	s[chunk_size_ - 1].link.next = nullptr;
	chunks_.push_back(s);
	return s;
    }

public:
    static strategy<T> *make(int chunk_size) {
	auto s = new chunk_strategy<T, Locker>(chunk_size);
	return static_cast<strategy<T>*>(s);
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
    
    waitable_strategy(int chunk_size, int chunk_limit):
	chunk_size_(chunk_size), chunk_limit_(chunk_limit) {
    }

    ~waitable_strategy() {
	freeall();
    }

    void freeall() {
	for (auto i: chunks_) {
	    delete[] i;
	}
	chunks_.clear();
    }

    c7::defer lock() {
	return cv_.lock();
    }

    item<T> *alloc() {
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

    void wait() {
	cv_.wait();
    }

    void notify() {
	cv_.notify_all();
    }

public:
    static strategy<T> *make(int chunk_size, int chunk_limit) {
	auto s = new waitable_strategy<T>(chunk_size, chunk_limit);
	return static_cast<strategy<T>*>(s);
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
mpool::mpool<T> mpool_chunk(int chunk_size)
{
    return mpool::mpool<T>(mpool::chunk_strategy<T, Locker>::make, chunk_size);
}

template <typename T>
mpool::mpool<T> mpool_waitable(int chunk_size, int chunk_limit)
{
    return mpool::mpool<T>(mpool::waitable_strategy<T>::make, chunk_size, chunk_limit);
}


} // namespace c7


#endif // c7mpool.hpp
