/*
 * c7coroutine.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <stdexcept>
#include <signal.h>	// MINSIGSTKSZ
#include <c7coroutine.hpp>


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
    c7context_ = new c7_context_data_t{};
}

coroutine::coroutine(size_t stack_b):
    stack_(new char[stack_b + _STACK_MIN_b])
{
    init_by_thread();
    c7context_ = new c7_context_data_t{};
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
    delete c7context_;
}

void coroutine::setup_context()
{
    c7_makecontext(c7context_, coroutine::entry_point, this);
}

void coroutine::entry_point(void *)	// static member
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
