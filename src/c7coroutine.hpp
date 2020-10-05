/*
 * c7coroutine.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=301766561
 */
#ifndef C7_COROUTINE_HPP_LOADED__
#define C7_COROUTINE_HPP_LOADED__
#include <c7common.hpp>


#include <c7delegate.hpp>
#include <memory>
#include <type_traits>
#include <ucontext.h>


namespace c7 {


//----------------------------------------------------------------------------
//                                coroutine
//----------------------------------------------------------------------------

class coroutine {
public:
    enum status_t {
	ALIVE, EXIT, ABORT
    };

private:
    std::unique_ptr<char[]> stack_;
    std::function<void()> target_;
    ucontext_t context_;
    status_t status_ = ALIVE;
    coroutine* from_ = nullptr;

    void setup_context();
    static void entry_point();
    void switch_to();
    static void exit_with(status_t);

public:
    c7::delegate<void> restore;

    coroutine();
    explicit coroutine(size_t stack_b);
    ~coroutine();

    coroutine(const coroutine&) = delete;
    coroutine(const coroutine&&) = delete;
    coroutine& operator=(const coroutine&) = delete;
    coroutine& operator=(const coroutine&&) = delete;

    void target(std::function<void()> target) {
	target_ = target;
	setup_context();
    }

    status_t yield() {
	switch_to();
	return coroutine::self()->from_->status_;
    }

    static coroutine* self();

    status_t status() {
	return status_;
    }

    coroutine* from() {
	return from_;
    }

    static void exit() {
	exit_with(EXIT);
    }

    static void abort() {
	exit_with(ABORT);
    }
};


//----------------------------------------------------------------------------
//                                generator
//----------------------------------------------------------------------------

template <typename T>
struct generator_scalar_traits {
    typedef T store_type;
    typedef T arg_type;
    typedef T value_type;
    typedef T* pointer;
    typedef T& reference;
    static inline store_type save(arg_type d) {
	return d;
    }
    static inline reference deref(store_type& d) {
	return d;
    }
};

template <typename T>
struct generator_reference_traits {
    typedef std::remove_reference_t<T> V;
    typedef V* store_type;
    typedef T arg_type;
    typedef T value_type;
    typedef V* pointer;
    typedef T reference;
    static inline store_type save(arg_type d) {
	return &d;
    }
    static inline reference deref(store_type d) {
	return *d;
    }
};

template <typename T>
struct generator_other_traits {
    typedef const T* store_type;
    typedef const T& arg_type;
    typedef T value_type;
    typedef const T* pointer;
    typedef const T& reference;
    static inline store_type save(arg_type d) {
	return &d;
    }
    static inline reference deref(store_type d) {
	return *d;
    }
};

class generator_base {
protected:
    static thread_local void *generator_;
};

template <typename T>
class generator: public generator_base {
public:
    typedef typename
    std::conditional_t<std::is_scalar_v<T>,
		       generator_scalar_traits<T>,
		       std::conditional_t<std::is_reference_v<T>,
					  generator_reference_traits<T>,
					  generator_other_traits<T>>> traits;

private:
    std::unique_ptr<coroutine> co_;
    typename traits::store_type data_;
    
public:
    class iterator {
    private:
	generator<T>& gen_;
	bool terminated_;

    public:
	typedef ptrdiff_t difference_type;
	typedef typename traits::value_type  value_type;
	typedef typename traits::pointer pointer;
	typedef typename traits::reference reference;
	typedef std::input_iterator_tag iterator_categroy;

	iterator(generator<T>& gen, bool terminated):
	    gen_(gen), terminated_(terminated) {
	    if (!terminated) {
		if (gen_.co_->yield() != coroutine::ALIVE) {
		    terminated_ = true;
		}
	    }
	}

	bool operator==(const iterator& rhs) const {
	    return (terminated_ && rhs.terminated_);
	}

	bool operator!=(const iterator& rhs) const {
	    return !operator==(rhs);
	}

	iterator& operator++() {
	    if (!terminated_) {
		if (gen_.co_->yield() != coroutine::ALIVE) {
		    terminated_ = true;
		}
	    }
	    return *this;
	}

	reference operator*() const {
	    if (terminated_) {
		throw std::out_of_range("Maybe end iterator.");
	    }
	    return traits::deref(gen_.data_);
	}
    };

public:
    generator(size_t stack_b, std::function<void()> func) {
	co_ = decltype(co_)(new coroutine(stack_b));
	co_->target(func);
	co_->restore += [this](){ generator_ = static_cast<void*>(this); };
    }

    ~generator() {}

    static void yield(typename traits::arg_type data) {
	auto gen = static_cast<generator<T>*>(generator_);
	gen->data_ = traits::save(data);
	gen->co_->from()->yield();
    }

    iterator begin() noexcept {
	return iterator(*this, false);
    }
    
    iterator end() noexcept {
	return iterator(*this, true);
    }

    bool is_success() {
	return (co_->status() != coroutine::ABORT);
    }
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7coroutine.hpp
