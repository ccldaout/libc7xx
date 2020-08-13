/*
 * c7utils.hpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __C7_UTILS_HPP_LOADED__
#define __C7_UTILS_HPP_LOADED__
#include "c7common.hpp"


#include "c7result.hpp"
#include <memory>
#include <pwd.h>
#include <sys/time.h>


namespace c7 {


/*----------------------------------------------------------------------------
                                loop assistant
----------------------------------------------------------------------------*/

template <typename C>
inline std::tuple<ptrdiff_t,ptrdiff_t,ptrdiff_t> loop_params(const C& c, bool ascendant)
{
    if (ascendant) {
	return std::make_tuple(0, (ptrdiff_t)c.size(), 1);
    } else {
	return std::make_tuple((ptrdiff_t)c.size() - 1, -1, -1);
    }
}


/*----------------------------------------------------------------------------
                      unique_ptr factory for malloc/free
----------------------------------------------------------------------------*/

template <typename T = void>
using unique_cptr = std::unique_ptr<T, void(*)(void*)>;

template <typename T = void>
std::unique_ptr<T, void(*)(void*)> make_unique_cptr(void *p = nullptr)
{
    return unique_cptr<T>(static_cast<T*>(p), std::free);
}

template <typename R>
struct result_traits<unique_cptr<R>> {
    static auto init() { return make_unique_cptr<R>(); }
};


/*----------------------------------------------------------------------------
                                     time
----------------------------------------------------------------------------*/

c7::usec_t time_us();
c7::usec_t sleep_us(c7::usec_t duration);
inline c7::usec_t sleep_ms(c7::usec_t duration_ms)
{
    return sleep_us(duration_ms * 1000) / 1000;
}
::timespec *timespec_from_duration(c7::usec_t duration, c7::usec_t reftime = 0);
inline ::timespec *mktimespec(c7::usec_t duration, c7::usec_t reftime = 0)
{
    return (duration < 0) ? nullptr : timespec_from_duration(duration, reftime);
}


/*----------------------------------------------------------------------------
                             get passwd user data
----------------------------------------------------------------------------*/

class passwd {
public:
    typedef unique_cptr<::passwd> passwd_ptr;

private:
    passwd_ptr pwd_;

public:
    passwd(): pwd_(nullptr, std::free) {}

    explicit passwd(passwd_ptr&& pwd): pwd_(std::move(pwd)) {}

    ::passwd* operator->() {
	return pwd_.get();
    }

    ::passwd& operator*() {
	return *pwd_;
    }

    static c7::result<passwd> by_name(const std::string& name);
    static c7::result<passwd> by_uid(::uid_t uid);
};


/*----------------------------------------------------------------------------
                            C array type iterator
----------------------------------------------------------------------------*/

template <typename T>
class c_array_iterator {
private:
    T **tpp_;
    ptrdiff_t idx_;

public:
    typedef ptrdiff_t difference_type;
    typedef T value_type;
    typedef T* pointer;
    typedef T& reference;
    typedef std::random_access_iterator_tag iterator_category;

    c_array_iterator(): tpp_(nullptr), idx_(0) {}
    c_array_iterator(T **tpp, size_t idx): tpp_(tpp), idx_((ptrdiff_t)idx) {}
    c_array_iterator(const c_array_iterator& o): tpp_(o.tpp_), idx_(o.idx_) {}
    c_array_iterator& operator=(const c_array_iterator& o) {
	tpp_ = o.tpp_;
	idx_ = o.idx_;
	return *this;
    }

    T& operator[](ptrdiff_t n) const {
	return (*tpp_)[idx_ + n];
    }

    T& operator*() const {
	return (*tpp_)[idx_];
    }

    T* operator->() const {
	return &(*tpp_)[idx_];
    }

    c_array_iterator<T>& operator++() {	// prefix
	idx_++;
	return *this;
    }

    c_array_iterator<T>& operator--() {	// prefix
	idx_--;
	return *this;
    }

    c_array_iterator<T> operator++(int) {	// postfix
	size_t i = idx_++;
	return c_array_iterator<T>(tpp_, i);
    }

    c_array_iterator<T> operator--(int) {	// postfix
	size_t i = idx_--;
	return c_array_iterator<T>(tpp_, i);
    }

    c_array_iterator<T>& operator+=(ptrdiff_t n) {
	idx_ += n;
	return *this;
    }

    c_array_iterator<T>& operator-=(ptrdiff_t n) {
	idx_ -= n;
	return *this;
    }

    c_array_iterator<T> operator+(ptrdiff_t n) const {
	return c_array_iterator<T>(tpp_, idx_ + n);
    }

    c_array_iterator<T> operator-(ptrdiff_t n) const {
	return c_array_iterator<T>(tpp_, idx_ - n);
    }

    ptrdiff_t operator-(const c_array_iterator<T>& o) const {
	return idx_ - o.idx_;
    }

    bool operator==(const c_array_iterator<T>& o) const {
	return idx_ == o.idx_;
    }

    bool operator!=(const c_array_iterator<T>& o) const {
	return idx_ != o.idx_;
    }

    bool operator<(const c_array_iterator<T>& o) const {
	return idx_ < o.idx_;
    }

    bool operator<=(const c_array_iterator<T>& o) const {
	return idx_ <= o.idx_;
    }

    bool operator>(const c_array_iterator<T>& o) const {
	return idx_ > o.idx_;
    }

    bool operator>=(const c_array_iterator<T>& o) const {
	return idx_ >= o.idx_;
    }
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7utils.hpp
