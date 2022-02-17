/*
 * c7thread/mutex.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include "mutex.hpp"
#include "_private.hpp"


namespace c7::thread {


mutex::mutex(bool recursive)
{
    pimpl = new mutex::impl(recursive);
}


mutex::~mutex()
{
    delete pimpl;
    pimpl = nullptr;
}


void mutex::_lock()
{
    (void)pimpl->_lock(true);
}


bool mutex::_trylock()
{
    return pimpl->_lock(false);
}


void mutex::unlock()
{
    pimpl->unlock();
}


} // namespace c7::thread
