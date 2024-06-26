/*
 * c7delegate.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=608487862
 */
#ifndef C7_DELEGATE_HPP_LOADED_
#define C7_DELEGATE_HPP_LOADED_
#include <c7common.hpp>


#include <algorithm>	// find_if
#include <atomic>
#include <functional>
#include <list>
#include <utility>	// std::pair


namespace c7 {


/*----------------------------------------------------------------------------
                                   delegate
----------------------------------------------------------------------------*/

// delegate traits

template <typename Return, typename... Args>
struct delegate_traits {};

template <typename... Args>
struct delegate_traits<bool, Args...> {
    typedef typename std::function<bool(Args...)> func_type;

    static bool apply(func_type f, Args... args) {
	return f(args...);
    }
};

template <typename... Args>
struct delegate_traits<void, Args...> {
    typedef typename std::function<void(Args...)> func_type;

    static bool apply(func_type f, Args... args) {
	f(args...);
	return false;
    }
};

// delegate

class delegate_base {
public:
    class id {
    private:
	static std::atomic<unsigned long> id_counter_;
	unsigned long id_;

    public:
	id(): id_(++id_counter_) {}
	bool operator==(const id& o) const {
	    return id_ == o.id_;
	}
    };
};

template <typename Return, typename... Args>
class delegate: public delegate_base {
public:
    typedef delegate_traits<Return, Args...> traits;
    typedef typename traits::func_type func_type;

    class proxy {
    private:
	delegate *principal_;

    public:
	explicit proxy(delegate& d): principal_(&d) {}

	id push_front(const func_type& f) {
	    return principal_->push_front(f);
	}
	id push_front(func_type&& f) {
	    return principal_->push_front(std::move(f));
	}

	id push_back(const func_type& f) {
	    return principal_->push_back(f);
	}
	id push_back(func_type&& f) {
	    return principal_->push_back(std::move(f));
	}

	template <typename F>
	proxy& operator+=(F&& f) {
	    push_back(std::forward<F>(f));
	    return *this;
	}

	void remove(id target_id) {
	    principal_->remove(target_id);
	}

	bool is_empty() const {
	    return principal_->is_empty();
	}

	size_t size() const {
	    return principal_->size();
	}
    };

    delegate(const delegate&) = delete;
    delegate& operator=(const delegate&) = delete;

    delegate() {}

    delegate(delegate&& o): funcs_(std::move(o.funcs_)) {}

    explicit delegate(const func_type& f) {
	push_back(f);
    }

    explicit delegate(const func_type&& f) {
	push_back(std::move(f));
    }

    delegate& operator=(delegate&& o) {
	if (this != &o) {
	    funcs_ = std::move(o.funcs_);
	}
	return *this;
    }

    id push_front(const func_type& f) {
	id new_id;
	funcs_.emplace_front(new_id, f);
	return new_id;
    }
    id push_front(func_type&& f) {
	id new_id;
	funcs_.emplace_front(new_id, std::move(f));
	return new_id;
    }

    id push_back(const func_type& f) {
	id new_id;
	funcs_.emplace_back(new_id, f);
	return new_id;
    }
    id push_back(func_type&& f) {
	id new_id;
	funcs_.emplace_back(new_id, std::move(f));
	return new_id;
    }

    template <typename F>
    delegate& operator+=(F&& f) {
	push_back(std::forward<F>(f));
	return *this;
    }

    void remove(id target_id) {
	auto it = std::find_if(funcs_.begin(), funcs_.end(),
			       [target_id](func_item& p) { return p.first == target_id; });
	if (it != funcs_.end()) {
	    funcs_.erase(it);
	}
    }

    bool is_empty() const {
	return funcs_.empty();
    }

    size_t size() const {
	return funcs_.size();
    }

    void clear() {
	return funcs_.clear();
    }

    //  true: some function  return true  (loop is breaked)
    // false:  all functions return false (loop is completed) OR empty loop
    bool operator()(Args... args) const {
	for (auto& [_, f]: funcs_) {
	    if (traits::apply(f, args...)) {
		return true;
	    }
	}
	return false;
    }

private:
    typedef std::pair<id, func_type> func_item;
    std::list<func_item> funcs_;
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7delegate.hpp
