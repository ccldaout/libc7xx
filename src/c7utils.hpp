/*
 * c7utils.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1803310993
 */
#ifndef C7_UTILS_HPP_LOADED__
#define C7_UTILS_HPP_LOADED__
#include <c7common.hpp>


#include <c7result.hpp>
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
                                reverse endian
----------------------------------------------------------------------------*/

namespace endian {

static inline void reverse(uint64_t& u) { __asm__("bswapq %0":"+r"(u)); }
static inline void reverse( int64_t& u) { __asm__("bswapq %0":"+r"(u)); }
static inline void reverse(uint32_t& u) { __asm__("bswapl %0":"+r"(u)); }
static inline void reverse( int32_t& u) { __asm__("bswapl %0":"+r"(u)); }
static inline void reverse(uint16_t& u) { __asm__("rorw $8,%0":"+r"(u)); }
static inline void reverse( int16_t& u) { __asm__("rorw $8,%0":"+r"(u)); }
static inline void reverse(uint8_t) {}
static inline void reverse( int8_t) {}
template <typename T> static inline T reverse_to(T u) { reverse(u); return u; }

} // namespace [c7::]endian


/*----------------------------------------------------------------------------
                                   spinlock
----------------------------------------------------------------------------*/

template <typename T>
static inline void spinlock_acquire(volatile T *p) {
    while (__sync_lock_test_and_set(p, 1) == 1);
}

template <typename T>
static inline void spinlock_release(volatile T *p) {
    __sync_lock_release(p);
}


/*----------------------------------------------------------------------------
                              simple raw storage
----------------------------------------------------------------------------*/

class storage {
public:
    storage() = default;
    explicit storage(size_t align): align_(align) {}
    ~storage();
    storage(const storage&) = delete;
    storage(storage&&);
    storage& operator=(const storage&) = delete;
    storage& operator=(storage&&);
    storage& copy_from(const storage&);
    storage copy_to();
    result<> reserve(size_t req);
    void reset();
    void clear();
    void set_align(size_t align) { align_ = align; }
    size_t align() { return align_; }
    size_t size() { return size_; }
    void *addr() { return addr_; }
    const void *addr() const { return addr_; }
    template <typename T> operator T*() { return static_cast<T*>(addr_); }
    template <typename T> operator const T*() const { return static_cast<T*>(addr_); }

private:
    void *addr_ = nullptr;
    size_t size_ = 0;
    size_t align_ = 1024;
};


/*----------------------------------------------------------------------------
                            C array type iterator
----------------------------------------------------------------------------*/

template <typename T>
class c_array_iterator {
private:
    T *top_;
    ptrdiff_t idx_;

public:
    typedef ptrdiff_t difference_type;
    typedef T value_type;
    typedef T* pointer;
    typedef T& reference;
    typedef std::random_access_iterator_tag iterator_category;

    c_array_iterator(): top_(nullptr), idx_(0) {}
    c_array_iterator(T *top, size_t idx): top_(top), idx_((ptrdiff_t)idx) {}
    c_array_iterator(const c_array_iterator& o): top_(o.top_), idx_(o.idx_) {}
    c_array_iterator& operator=(const c_array_iterator& o) {
	top_ = o.top_;
	idx_ = o.idx_;
	return *this;
    }

    T& operator[](ptrdiff_t n) const {
	return top_[idx_ + n];
    }

    T& operator*() const {
	return top_[idx_];
    }

    T* operator->() const {
	return &top_[idx_];
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
	return c_array_iterator<T>(top_, i);
    }

    c_array_iterator<T> operator--(int) {	// postfix
	size_t i = idx_--;
	return c_array_iterator<T>(top_, i);
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
	return c_array_iterator<T>(top_, idx_ + n);
    }

    c_array_iterator<T> operator-(ptrdiff_t n) const {
	return c_array_iterator<T>(top_, idx_ - n);
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
       movable and non-copyable object warapper (for functional object)
----------------------------------------------------------------------------*/

template <typename T>
class movable_capture: private T {
public:
    movable_capture(T& obj): T(std::move(obj)) {}
    movable_capture(const movable_capture& o):
	T(std::move(static_cast<T&&>(const_cast<movable_capture&>(o)))) {
    }
    movable_capture& operator=(const movable_capture& o) {
	return T::operator=(static_cast<T&&>(const_cast<movable_capture&>(o)));
    }
    movable_capture(movable_capture&& o):
	T(std::move(static_cast<T&&>(o))) {
    }
    movable_capture& operator=(movable_capture&& o) {
	return T::operator=(static_cast<T&&>(o));
    }
    T&& unwrap() { return static_cast<T&&>(*this); }
    T&& unwrap() const { return const_cast<T&&>(static_cast<const T&&>(*this)); }
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7utils.hpp
