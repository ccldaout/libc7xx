/*
 * c7utils/storage.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_UTILS_STORAGE_HPP_LOADED_
#define C7_UTILS_STORAGE_HPP_LOADED_
#include <c7common.hpp>


#include <c7result.hpp>


namespace c7 {


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


} // namespace c7


#endif // c7utils/storage.hpp
