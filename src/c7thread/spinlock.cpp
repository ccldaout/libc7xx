/*
 * c7thread/spinlock.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include "spinlock.hpp"
#include "_private.hpp"


namespace c7::thread {


class spinlock::impl {
private:
    pthread_spinlock_t m_;

public:
    impl() {
	(void)pthread_spin_init(&m_, PTHREAD_PROCESS_PRIVATE);
    }

    ~impl() {
	(void)pthread_spin_destroy(&m_);
    }

    bool _lock(bool wait) {
	int ret = wait ? pthread_spin_lock(&m_) : pthread_spin_trylock(&m_);
	if (ret != C7_SYSOK) {
	    if (ret == EBUSY) {
		return false;
	    }
	    if (process_alive) {
		throw thread_error(ret, "pthread_spinlock_[try]lock");
	    }
	}
	return true;
    }

    void unlock() {
	(void)pthread_spin_unlock(&m_);
    }

    pthread_spinlock_t& pthread_spinlock_() {
	return m_;
    }
};


spinlock::spinlock()
{
    pimpl = new spinlock::impl();
}


spinlock::~spinlock()
{
    delete pimpl;
    pimpl = nullptr;
}


void spinlock::_lock()
{
    (void)pimpl->_lock(true);
}


bool spinlock::_trylock()
{
    return pimpl->_lock(false);
}


void spinlock::unlock()
{
    pimpl->unlock();
}


} // namespace c7::thread
