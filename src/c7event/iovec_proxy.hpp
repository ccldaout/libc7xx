/*
 * c7event/iovec_proxy.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_IOVEC_PROXY_HPP_LOADED__
#define C7_EVENT_IOVEC_PROXY_HPP_LOADED__
#include <c7common.hpp>


#include <sys/uio.h>
#include <cstring>


namespace c7::event {


class iovec_proxy {
private:
    ::iovec *iov_;
    typedef decltype(std::declval<::iovec>().iov_len) len_t;

    class iov_len_t {
    public:
	explicit iov_len_t(::iovec *&iov): iov_(iov) {}
	iov_len_t& operator=(len_t n) { iov_->iov_len = n; return *this; }
	operator len_t() { return iov_->iov_len; }
	template <typename T> operator T() { return static_cast<T>(iov_->iov_len); }
	auto print_as() const { return iov_->iov_len; }
    private:
	::iovec *&iov_;
    };

    class iov_base_t {
    public:
	explicit iov_base_t(::iovec *&iov): iov_(iov) {}
	iov_base_t& operator=(void *p) {
	    iov_->iov_base = p;
	    return *this;
	}
	iov_base_t& operator=(const void *p) {
	    iov_->iov_base = const_cast<void*>(p);
	    return *this;
	}
	iov_base_t& operator=(const char *s) {
	    iov_->iov_base = const_cast<char*>(s);
	    iov_->iov_len = std::strlen(s) + 1;
	    return *this;
	}
	iov_base_t& operator=(const std::string& s) {
	    iov_->iov_base = const_cast<char*>(s.c_str());
	    iov_->iov_len = s.size() + 1;
	    return *this;
	}
	template <typename T> iov_base_t& operator=(T *p) {
	    iov_->iov_base = p;
	    iov_->iov_len = sizeof(T);
	    return *this;
	}
	template <typename T> iov_base_t& operator=(const T *p) {
	    return operator=(const_cast<T*>(p));
	}
	template <typename T> iov_base_t& operator=(const std::vector<T>& v) {
	    iov_->iov_base = const_cast<void*>(static_cast<const void*>(v.data()));
	    iov_->iov_len = sizeof(T) * v.size();
	    return *this;
	}
	template <typename T, size_t N> iov_base_t& operator=(const std::array<T, N>& v) {
	    iov_->iov_base = const_cast<void*>(static_cast<const void*>(v.data()));
	    iov_->iov_len = sizeof(T) * N;
	    return *this;
	}

	operator void* () {
	    return iov_->iov_base;
	}
	template <typename T> operator T* () {
	    return as<T>();
	}
	template <typename T> T* as() {
	    return static_cast<T*>(iov_->iov_base);
	}

	auto print_as() const {
	    return iov_->iov_base;
	}
    private:
	::iovec *&iov_;
    };

public:
    iov_len_t iov_len;
    iov_base_t iov_base;

    explicit iovec_proxy(::iovec *iov):
	iov_(iov), iov_len(iov_), iov_base(iov_) {}
    explicit iovec_proxy(const ::iovec *iov):
	iov_(const_cast<::iovec*>(iov)), iov_len(iov_), iov_base(iov_) {}
    
    ::iovec* operator&() { return iov_; }
    iovec_proxy& operator++() { iov_++; return *this; }
    iovec_proxy& operator--() { iov_--; return *this; }
    iovec_proxy& operator+=(int n) { iov_ += n; return *this; }
    iovec_proxy& operator-=(int n) { iov_ -= n; return *this; }

    template <typename T>
    c7::result<> strict_ptr(T*& p) {
	if (iov_->iov_base != nullptr && iov_->iov_len == sizeof(T)) {
	    p = static_cast<T*>(iov_->iov_base);
	    return c7result_ok();
	} else {
	    p = nullptr;
	    return c7result_err(EINVAL, "type size mismatch: require:%{}, actual:%{}",
				sizeof(T), iov_->iov_len);
	}
    }
    template <typename T>
    c7::result<> strict_ptr(T*& p, size_t& n) {
	if (iov_->iov_base != nullptr && (iov_->iov_len % sizeof(T)) == 0) {
	    p = static_cast<T*>(iov_->iov_base);
	    n = iov_->iov_len / sizeof(T);
	    return c7result_ok();
	} else {
	    p = nullptr;
	    n = 0;
	    return c7result_err(EINVAL, "type size mismatch: require:%{}xN, actual:%{}",
				sizeof(T), iov_->iov_len);
	}
    }
    template <typename T>
    c7::result<> relaxed_ptr(T*& p) {
	if (iov_->iov_base != nullptr && iov_->iov_len > sizeof(T)) {
	    p = static_cast<T*>(iov_->iov_base);
	    return c7result_ok();
	} else {
	    p = nullptr;
	    return c7result_err(EINVAL, "type size mismatch: require:%{}, actual:%{}",
				sizeof(T), iov_->iov_len);
	}
    }
    template <typename T>
    c7::result<> relaxed_ptr(T*& p, size_t& n) {
	if (iov_->iov_base != nullptr && iov_->iov_len > sizeof(T)) {
	    p = static_cast<T*>(iov_->iov_base);
	    n = iov_->iov_len / sizeof(T);
	    return c7result_ok();
	} else {
	    p = nullptr;
	    n = 0;
	    return c7result_err(EINVAL, "type size mismatch: require:%{}, actual:%{}",
				sizeof(T), iov_->iov_len);
	}
    }
};


} // namespace c7::event


#endif // c7event/iovec_proxy.hpp
