/*
 * c7utils/spinlock.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_UTILS_SPINLOCK_HPP_LOADED_
#define C7_UTILS_SPINLOCK_HPP_LOADED_
#include <c7common.hpp>


namespace c7 {


template <typename T>
static inline void spinlock_acquire(volatile T *p) {
    while (__sync_lock_test_and_set(p, 1) == 1);
}

template <typename T>
static inline void spinlock_release(volatile T *p) {
    __sync_lock_release(p);
}

template <typename T>
class spinlock {
public:
    explicit spinlock(T& target): p_(&target) {
	while (__sync_lock_test_and_set(p_, 1) == 1);
    }

    void release() {
	__sync_lock_release(p_);
	p_ = nullptr;
    }

    ~spinlock() {
	if (p_ != nullptr) {
	    __sync_lock_release(p_);
	}
    }

private:
    T *p_;
};


} // namespace c7


#endif // c7utils/spinlock.hpp
