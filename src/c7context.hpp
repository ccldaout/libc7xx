/*
 * c7context.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_CONTEXT_HPP_LOADED_
#define C7_CONTEXT_HPP_LOADED_
#include <c7common.hpp>


#if defined(__x86_64) || defined(__aarch64__)
# define USE_C7_CONTEXT
#endif

#if defined(USE_C7_CONTEXT)
struct c7_context_data_t;
#else
# include <ucontext.h>
using c7_context_data_t = ucontext_t;
#endif
using c7_context_t = c7_context_data_t*;


#if defined(USE_C7_CONTEXT)

struct c7_context_data_t {
    void *stack_area;
    size_t stack_size;
# if defined(__x86_64)
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rbx;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rdi;	// 1st argument for entry_point()
# elif defined(__aarch64__)
    uint64_t d8;
    uint64_t d9;
    uint64_t d10;
    uint64_t d11;
    uint64_t d12;
    uint64_t d13;
    uint64_t d14;
    uint64_t d15;
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t x30;	// PC
    uint64_t sp;
    uint64_t x29;
    uint64_t x0;	// 1st argument for entry_point()
# else
#   error "Not implemented for this architecture"
# endif
};

#endif // USE_C7_CONTEXT


extern "C" {
int c7_getcontext(c7_context_t ctx);
void c7_makecontext(c7_context_t ctx, void (*func)(void*), void*);
int c7_swapcontext(c7_context_t c_ctx, const c7_context_t t_ctx);
}


#endif // c7context.hpp
