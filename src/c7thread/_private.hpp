/*
 * c7thread/_private.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1557099975
 */
#ifndef C7_THREAD_uPRIVATE_HPP_LOADED_
#define C7_THREAD_uPRIVATE_HPP_LOADED_
#include <c7common.hpp>


#include <pthread.h>
#include <system_error>
#include "mutex.hpp"


namespace c7::thread {


extern bool process_alive;


struct thread_error: public std::system_error {
    thread_error(int err, const char *s):
	std::system_error(std::error_code(err, std::generic_category()), s) {}
};


class mutex::impl {
private:
    pthread_mutex_t m_;

public:
    explicit impl(bool recursive) {
	pthread_mutexattr_t attr;
	(void)pthread_mutexattr_init(&attr);
	if (recursive) {
	    (void)pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	}
	(void)pthread_mutex_init(&m_, &attr);
    }

    ~impl() {
	(void)pthread_mutex_destroy(&m_);
    }

    bool _lock(bool wait) {
	int ret = wait ? pthread_mutex_lock(&m_) : pthread_mutex_trylock(&m_);
	if (ret != C7_SYSOK) {
	    if (ret == EBUSY) {
		return false;
	    }
	    if (process_alive) {
		throw thread_error(ret, "pthread_mutex_[try]lock");
	    }
	}
	return true;
    }

    void unlock() {
	(void)pthread_mutex_unlock(&m_);
    }

    pthread_mutex_t& pthread_mutex_() {
	return m_;
    }
};


} // namespace c7::hread


#endif // c7thread/_private.hpp
