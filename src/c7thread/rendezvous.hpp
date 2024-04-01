/*
 * c7thread/rendezvous.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=192335276
 */
#ifndef C7_THREAD_RENDEZVOUS_HPP_LOADED_
#define C7_THREAD_RENDEZVOUS_HPP_LOADED_
#include <c7common.hpp>


#include <c7thread/condvar.hpp>


namespace c7::thread {


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


} // namespace c7::thread


#endif // c7thread/rendezvous.hpp
