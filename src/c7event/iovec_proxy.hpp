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
    ::iovec& iov_;
    typedef decltype(std::declval<::iovec>().iov_len) len_t;

    class iov_len_t {
    public:
	explicit iov_len_t(::iovec &iov): iov_(iov) {}
	iov_len_t& operator=(len_t n) { iov_.iov_len = n; return *this; }
	operator len_t() { return iov_.iov_len; }
	template <typename T> operator T() { return static_cast<T>(iov_.iov_len); }
	auto print_as() const { return iov_.iov_len; }
    private:
	::iovec &iov_;
    };

    class iov_base_t {
    public:
	explicit iov_base_t(::iovec &iov): iov_(iov) {}
	iov_base_t& operator=(void *p) {
	    iov_.iov_base = p;
	    return *this;
	}
	iov_base_t& operator=(const void *p) {
	    iov_.iov_base = const_cast<void*>(p);
	    return *this;
	}
	iov_base_t& operator=(const char *s) {
	    iov_.iov_base = const_cast<char*>(s);
	    iov_.iov_len = std::strlen(s) + 1;
	    return *this;
	}
	iov_base_t& operator=(const std::string& s) {
	    iov_.iov_base = const_cast<char*>(s.c_str());
	    iov_.iov_len = s.size() + 1;
	    return *this;
	}
	template <typename T> iov_base_t& operator=(T *p) {
	    iov_.iov_base = p;
	    iov_.iov_len = sizeof(T);
	    return *this;
	}
	template <typename T> iov_base_t& operator=(const T *p) {
	    return operator=(const_cast<T*>(p));
	}
	template <typename T> iov_base_t& operator=(const std::vector<T>& v) {
	    iov_.iov_base = const_cast<void*>(static_cast<const void*>(v.data()));
	    iov_.iov_len = sizeof(T) * v.size();
	    return *this;
	}
	template <typename T, size_t N> iov_base_t& operator=(const std::array<T, N>& v) {
	    iov_.iov_base = const_cast<void*>(static_cast<const void*>(v.data()));
	    iov_.iov_len = sizeof(T) * N;
	    return *this;
	}

	operator void* () {
	    return iov_.iov_base;
	}
	operator const void* () const {
	    return iov_.iov_base;
	}
	template <typename T> operator T* () {
	    return as<T>();
	}
	template <typename T> operator const T* () const {
	    return as<T>();
	}
	template <typename T> T* as() {
	    return static_cast<T*>(iov_.iov_base);
	}
	template <typename T> const T* as() const {
	    return static_cast<T*>(iov_.iov_base);
	}

	auto print_as() const {
	    return iov_.iov_base;
	}
    private:
	::iovec &iov_;
    };

    c7::result<> size_error(size_t elm_size) const {
	return c7result_err(EINVAL, "type size mismatch: sizeof():%{}, actual:%{}",
			    elm_size, iov_.iov_len);
    }

public:
    iov_len_t iov_len;
    iov_base_t iov_base;

    explicit iovec_proxy(::iovec& iov):
	iov_(iov), iov_len(iov_), iov_base(iov_) {}
    explicit iovec_proxy(const ::iovec& iov):
	iov_(const_cast<::iovec&>(iov)), iov_len(iov_), iov_base(iov_) {}
    
    ::iovec* operator&() { return &iov_; }

    template <typename T>
    c7::result<> strict_ptr(T*& p) const {
	p = static_cast<T*>(iov_.iov_base);
	if (iov_.iov_len == sizeof(T)) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T));
	}
    }
    template <typename T>
    c7::result<> strict_ptr_nullable(T*& p) const {
	p = static_cast<T*>(iov_.iov_base);
	if (iov_.iov_len == sizeof(T) || p == nullptr) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T));
	}
    }
    template <typename T>
    c7::result<> strict_ptr(T*& p, size_t& n) const {
	p = static_cast<T*>(iov_.iov_base);
	n = iov_.iov_len / sizeof(T);
	if (n > 0 && (iov_.iov_len % sizeof(T)) == 0) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T));
	}
    }
    template <typename T>
    c7::result<> strict_ptr_nullable(T*& p, size_t& n) const {
	p = static_cast<T*>(iov_.iov_base);
	n = iov_.iov_len / sizeof(T);
	if (n >= 0 && (iov_.iov_len % sizeof(T)) == 0) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T));
	}
    }
    template <typename T>
    c7::result<> strict_ptr_just(T*& p, size_t n) const {
	p = static_cast<T*>(iov_.iov_base);
	if ((iov_.iov_len / sizeof(T)) == n) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T));
	}
    }
    template <typename T>
    c7::result<> relaxed_ptr(T*& p) const {
	p = static_cast<T*>(iov_.iov_base);
	if (iov_.iov_len > sizeof(T)) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T));
	}
    }
    template <typename T>
    c7::result<> relaxed_ptr_nullable(T*& p) const {
	p = static_cast<T*>(iov_.iov_base);
	if (iov_.iov_len > sizeof(T) || p == nullptr) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T));
	}
    }
};


} // namespace c7::event


#endif // c7event/iovec_proxy.hpp
