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
}

coroutine::coroutine(size_t stack_b):
    stack_(new char[stack_b + _STACK_MIN_b])
{
    init_by_thread();
    if (::getcontext(&context_) != C7_SYSOK) {
	stack_.reset();
	throw std::runtime_error("getcontext failure");
    }
    context_.uc_stack.ss_sp = static_cast<void*>(stack_.get());
    context_.uc_stack.ss_size = stack_b + _STACK_MIN_b;
    context_.uc_stack.ss_flags = 0;
    context_.uc_link = &context_;
}
	
coroutine::~coroutine()
{
    stack_.reset();
}
    
void coroutine::setup_context()
{
    ::makecontext(&context_, coroutine::entry_point, 0);
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
    ::swapcontext(&from_->context_, &context_);
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
