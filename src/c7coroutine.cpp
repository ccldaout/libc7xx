/*
 * c7coroutine.cpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include "c7coroutine.hpp"
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

coroutine::coroutine():
    yield_from_(nullptr)
{
    init_by_thread();
}

coroutine::coroutine(size_t stack_b):
    stack_(new char[stack_b + _STACK_MIN_b]), yield_from_(nullptr)
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
    yield_data_.empty();
    stack_.reset();
}
    
void coroutine::setup_context()
{
    ::makecontext(&context_, coroutine::entry_point, 0);
}

void coroutine::entry_point()		// static member
{
    current->func_();
    current->exit();
}

void coroutine::switch_this_from(coroutine* from)
{
    current = this;
    yield_from_ = from;
    ::swapcontext(&from->context_, &context_);
}

coroutine* coroutine::self()		// static member
{
    init_by_thread();
    return current;
}

void coroutine::exit_with(yield_status status)
{
    for (;;) {
	auto next = yield_from_;
	next->yield_status_ = status;
	next->yield_data_.empty();
	next->switch_this_from(this);
    }
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7
