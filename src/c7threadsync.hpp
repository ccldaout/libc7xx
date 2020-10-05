/*
 * c7threadsync.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=192335276
 */
#ifndef C7_THREADSYNC_HPP_LOADED__
#define C7_THREADSYNC_HPP_LOADED__
#include <c7common.hpp>


#include <c7thread.hpp>


namespace c7 {
namespace thread {


/*----------------------------------------------------------------------------
                                 thread group
----------------------------------------------------------------------------*/

class group {
private:
    class impl;
    impl* pimpl;

public:
    c7::delegate<void, proxy>::proxy on_any_finish;
    c7::delegate<void       >::proxy on_all_finish;
    
    typedef std::vector<proxy>::iterator iterator;

    group(const group&) = delete;
    group& operator=(const group&) = delete;

    group();
    group(group&&);
    group& operator=(group&&);
    ~group();

    result<> add(thread& th);
    result<> start();
    void wait();

    size_t size();
    iterator begin();
    iterator end();
};


/*----------------------------------------------------------------------------
                          synchronization - counter
----------------------------------------------------------------------------*/

class counter {
private:
    condvar c_;
    int counter_;

public:
    c7::delegate<void> on_zero;

    counter(const counter&) = delete;
    counter(counter&&) = delete;
    counter& operator=(const counter&) = delete;
    counter& operator=(counter&&) = delete;

    explicit counter(int init_count = 0);
    
    int count();
    void reset(int count);
    void up(int n = 1);
    bool down(c7::usec_t timeout = -1);
    bool wait_atleast(int expect, c7::usec_t timeout = -1);
    bool wait_just(int expect, c7::usec_t timeout = -1);
};


/*----------------------------------------------------------------------------
                           synchronization - event
----------------------------------------------------------------------------*/

class event {
private:
    condvar c_;
    bool event_;

    bool wait_unified(c7::usec_t timeout, bool clear);

public:
    c7::delegate<void> on_set;

    event(const event&) = delete;
    event(event&&) = delete;
    event& operator=(const event&) = delete;
    event& operator=(event&&) = delete;

    explicit event(bool init = false);
    
    bool is_set();
    void set();
    void clear();
    bool wait(c7::usec_t timeout = -1) {
	return wait_unified(timeout, false);
    }
    bool wait_and_clear(c7::usec_t timeout = -1) {
	return wait_unified(timeout, true);
    }
};


/*----------------------------------------------------------------------------
                            synchronization - mask
----------------------------------------------------------------------------*/

class mask {
private:
    condvar c_;
    uint64_t mask_;

public:
    mask(const mask&) = delete;
    mask(mask&&) = delete;
    mask& operator=(const mask&) = delete;
    mask& operator=(mask&&) = delete;

    explicit mask(uint64_t ini_mask);
    
    uint64_t get();
    void clear();
    void on(uint64_t on_mask);
    void off(uint64_t off_mask);
    bool change(std::function<void(uint64_t& in_out)> func);
    uint64_t wait(std::function<uint64_t(uint64_t& in_out)> func, c7::usec_t timeout = -1);

    void change(uint64_t on_mask, uint64_t off_mask) {
	change([on_mask,off_mask](uint64_t& in_out){
		in_out = (in_out|on_mask) & ~off_mask;
	    });
    }

    uint64_t wait_all(uint64_t expect_all, uint64_t clear, c7::usec_t timeout = -1) {
	return wait([expect_all,clear](uint64_t& in_out){
		if ((expect_all & in_out) == expect_all) {
		    in_out &= ~clear;
		    return expect_all;
		}
		return 0UL;
	    }, timeout);
    }

    uint64_t wait_any(uint64_t expect_any, uint64_t clear, c7::usec_t timeout = -1) {
	return wait([expect_any,clear](uint64_t& in_out){
		uint64_t ret = (expect_any & in_out);
		if (ret != 0)
		    in_out &= ~clear;
		return ret;
	    }, timeout);
    }
};


/*----------------------------------------------------------------------------
                         synchronization - rendezvous
----------------------------------------------------------------------------*/

class rendezvous {
private:
    condvar c_;
    uint64_t id_;
    int n_entry_;
    int waiting_;

public:
    rendezvous(const rendezvous&) = delete;
    rendezvous(rendezvous&&) = delete;
    rendezvous& operator=(const rendezvous&) = delete;
    rendezvous& operator=(rendezvous&&) = delete;
    
    explicit rendezvous(int n_entry);

    bool wait(c7::usec_t timeout = -1);
    void abort();
    void reset();
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace thread
} // namespace c7


#endif // c7threadsync.hpp
