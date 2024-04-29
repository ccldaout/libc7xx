/*
 * c7coroutine.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7coroutine.hpp>
#include <stdexcept>
#include <signal.h>	// MINSIGSTKSZ


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
#elif defined(__aarch64__)
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
#else
# error "Not implemented for this architecture"
#endif
};

extern "C" {
int c7_getcontext(c7_context_t ctx);
void c7_makecontext(c7_context_t ctx, void (*func)(), int);
int c7_swapcontext(c7_context_t c_ctx, const c7_context_t t_ctx);
}

#else

static int c7_getcontext(c7_context_t ctx)
{
    return ::getcontext(ctx);
}

static void c7_makecontext(c7_context_t ctx, void (*func)(), int)
{
    ::makecontext(ctx, func, 0);
}

int c7_swapcontext(c7_context_t c_ctx, const c7_context_t t_ctx)
{
    return ::swapcontext(c_ctx, t_ctx);
}

#endif


namespace c7 {


#define _STACK_MIN_b	SIGSTKSZ


//----------------------------------------------------------------------------
//                                coroutine
//----------------------------------------------------------------------------

static thread_local coroutine main_routine;
static thread_local coroutine* current = nullptr;

static void inline init_by_thread()
{
    if (current == nullptr) {
	current = &main_routine;
    }
}

coroutine::coroutine()
{
    init_by_thread();
#if defined(USE_C7_CONTEXT)
    c7context_ = new c7_context_data_t{};
#else
    c7context_ = &ucontext_;
#endif
}

coroutine::coroutine(size_t stack_b):
    stack_(new char[stack_b + _STACK_MIN_b])
{
    init_by_thread();
#if defined(USE_C7_CONTEXT)
    c7context_ = new c7_context_data_t{};
#else
    c7context_ = &ucontext_;
#endif
    if (c7_getcontext(c7context_) != C7_SYSOK) {
	stack_.reset();
	throw std::runtime_error("c7_getcontext failure");
    }
#if defined(USE_C7_CONTEXT)
    c7context_->stack_area = static_cast<void*>(stack_.get());
    c7context_->stack_size = stack_b + _STACK_MIN_b;
#else
    c7context_->uc_stack.ss_sp = static_cast<void*>(stack_.get());
    c7context_->uc_stack.ss_size = stack_b + _STACK_MIN_b;
    c7context_->uc_stack.ss_flags = 0;
    c7context_->uc_link = &ucontext_;
#endif
}

coroutine::~coroutine()
{
    stack_.reset();
#if defined(USE_C7_CONTEXT)
    delete c7context_;
#endif
}

void coroutine::setup_context()
{
    c7_makecontext(c7context_, coroutine::entry_point, 0);
}

void coroutine::entry_point()		// static member
{
    current->target_();
    coroutine::exit();
}

void coroutine::switch_to()
{
    from_ = current;
    current = this;
    restore();
    c7_swapcontext(from_->c7context_, c7context_);
}

coroutine* coroutine::self()		// static member
{
    init_by_thread();
    return current;
}

void coroutine::exit_with(status_t s)	// static member
{
    for (;;) {
	current->status_ = s;
	current->from_->switch_to();
    }
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

thread_local void *generator_base::generator_;


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7
