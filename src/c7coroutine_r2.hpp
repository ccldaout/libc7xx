/*
 * c7coroutine_r2.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_COROUTINE_R2_HPP_LOADED_
#define C7_COROUTINE_R2_HPP_LOADED_
#include <c7common.hpp>


#include <c7context.hpp>
#include <c7typefunc.hpp>


namespace c7::r2 {


class co_base {
public:
    co_base() {}
    co_base(const co_base& co) = delete;
    co_base(co_base&& co) = delete;
    co_base& operator=(const co_base& co) = delete;
    co_base& operator=(co_base&& co) = delete;

    virtual ~co_base() {}

    void init(void *stack_addr, size_t stack_size) {
	ctx_.stack_area = stack_addr;
	ctx_.stack_size = stack_size;
    }

    bool start() {
	c7_getcontext(&ctx_);
	c7_makecontext(&ctx_, co_base::entry_point0, this);
	alive_ = true;
	return resume();
    }

    bool resume() {
	co_base from;
	from.alive_ = true;
	return from.co_switch(*this);
    }

protected:
    virtual void entry_point() {}

    bool co_switch() {
	return co_switch(*co_from_);
    }

    bool co_switch(co_base& co) {
	if (!co.is_alive()) {
	    return false;
	}
	co.co_from_ = this;
	c7_swapcontext(&ctx_, &co.ctx_);
	return true;
    }

    co_base *co_from() {
	return co_from_;
    }

    bool is_alive() {
	return alive_;
    }

private:
    c7_context_data_t ctx_{nullptr, 0};
    co_base *co_from_ = nullptr;
    bool alive_ = false;

    void entry_point1() {
	entry_point();
	alive_ = false;
	if (!co_switch()) {
	    throw std::runtime_error("coroutine can't switch anywhere");
	}
    }

    static void entry_point0(void *self) {
	reinterpret_cast<co_base*>(self)->entry_point1();
    }
};


template <size_t StackSize = 1024>
class coroutine: public co_base {
public:
    coroutine() {
	init(stack_.data(), StackSize);
    }

private:
    std::array<uint64_t, StackSize/sizeof(uint64_t)> stack_;
    c7_context_data_t ctx_;
};


} // namespace c7::r2


#endif // c7coroutine_r2.hpp
