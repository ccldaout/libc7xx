/*
 * c7thread/group.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=192335276
 */
#ifndef C7_THREAD_GROUP_HPP_LOADED__
#define C7_THREAD_GROUP_HPP_LOADED__
#include <c7common.hpp>


#include <c7thread/thread.hpp>
#include <c7delegate.hpp>


namespace c7::thread {


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


} // namespace c7::thread


#endif // c7thread/group.hpp
