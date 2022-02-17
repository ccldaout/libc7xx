/*
 * c7thread/condvar.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include "condvar.hpp"
#include "_private.hpp"


namespace c7::thread {


class condvar::impl {
private:
    pthread_cond_t c_ = PTHREAD_COND_INITIALIZER;
    mutex::impl* mimpl_;
    bool allocated_;

public:
    impl(mutex* mutex) {
	allocated_ = (mutex == nullptr);
	if (mutex == nullptr) {
	    mimpl_ = new mutex::impl(false);
	} else {
	    mimpl_ = mutex->pimpl;
	}
    }

    ~impl() {
	(void)pthread_cond_destroy(&c_);
	if (allocated_) {
	    delete mimpl_;
	}
    }

    bool _lock(bool wait) {
	return mimpl_->_lock(wait);
    }

    void unlock() {
	mimpl_->unlock();
    }

    bool wait(const ::timespec *timeout_abs) {
	int ret;
	if (timeout_abs == nullptr) {
	    ret = pthread_cond_wait(&c_, &mimpl_->__pthread_mutex());
	} else {
	    do {
		ret = pthread_cond_timedwait(&c_, &mimpl_->__pthread_mutex(), timeout_abs);
	    } while (ret == EINTR);
	}
	if (ret != C7_SYSOK) {
	    if (ret == ETIMEDOUT) {
		return false;
	    }
	    if (process_alive) {
		throw thread_error(ret, "pthread_cond_[timed]wait");
	    }
	}
	return true;
    }

    void notify() {
	int ret = pthread_cond_signal(&c_);
	if (ret != C7_SYSOK) {
	    if (process_alive) {
		throw thread_error(ret, "pthread_cond_signal");
	    }
	}
    }

    void notify_all() {
	int ret = pthread_cond_broadcast(&c_);
	if (ret != C7_SYSOK) {
	    if (process_alive) {
		throw thread_error(ret, "pthread_cond_broadcast");
	    }
	}
    }
};


condvar::condvar()
{
    pimpl = new condvar::impl(nullptr);
}


condvar::condvar(mutex& mutex)
{
    pimpl = new condvar::impl(&mutex);
}


condvar::~condvar()
{
    delete pimpl;
    pimpl = nullptr;
}


void condvar::_lock()
{
    (void)pimpl->_lock(true);
}


bool condvar::_trylock()
{
    return pimpl->_lock(false);
}


void condvar::unlock()
{
    pimpl->unlock();
}


void condvar::wait()
{
    pimpl->wait(nullptr);
}


bool condvar::wait(const ::timespec* timeout_abs)
{
    return pimpl->wait(timeout_abs);
}


void condvar::notify()
{
    pimpl->notify();
}


void condvar::notify_all()
{
    pimpl->notify_all();
}


} // namespace c7::thread
