/*
 * c7signal.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=661593354
 */
#ifndef C7_SIGNAL_HPP_LOADED_
#define C7_SIGNAL_HPP_LOADED_
#include <c7common.hpp>


#include <c7defer.hpp>
#include <c7result.hpp>
#include <signal.h>


namespace c7 {
namespace signal {


c7::result<void(*)(int)>
handle(int sig, void (*handler)(int), const ::sigset_t& sigmask_on_call);

inline c7::result<void(*)(int)>
handle(int sig, void (*handler)(int))
{
    ::sigset_t sigs;
    (void)::sigemptyset(&sigs);
    (void)::sigaddset(&sigs, sig);
    return handle(sig, handler, sigs);
}

::sigset_t mask(int how, const ::sigset_t& sigs);

::sigset_t set(const ::sigset_t& sigs);

::sigset_t unblock(const ::sigset_t& sigs);

::sigset_t block(const ::sigset_t& sigs);

template <typename... Args>
inline ::sigset_t block(::sigset_t& sigs, int sig1, Args... args)
{
    (void)::sigaddset(&sigs, sig1);
    return block(sigs, args...);
}

template <typename... Args>
[[nodiscard]]
inline c7::defer block(int sig1, Args... args)
{
    ::sigset_t sigs;
    ::sigemptyset(&sigs);
    ::sigaddset(&sigs, sig1);
    auto o_set = block(sigs, args...);
    return c7::defer(set, o_set);
}

[[nodiscard]]
c7::defer block(void);

inline void restore(const ::sigset_t& sigs)
{
    mask(SIG_SETMASK, sigs);
}


} // namespace signal
} // namespace c7


#endif // c7signal.hpp
