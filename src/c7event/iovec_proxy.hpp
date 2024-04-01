/*
 * c7event/iovec_proxy.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_IOVEC_PROXY_HPP_LOADED_
#define C7_EVENT_IOVEC_PROXY_HPP_LOADED_
#include <c7common.hpp>


#include <sys/uio.h>
#include <cstring>
#include <string_view>
#include <c7slice.hpp>
#include <c7result.hpp>


#if defined(C7_IOVEC_PROXY_DEBUG)
# define debug_(v)	c7echo(v)
#else
# define debug_(v)
#endif


namespace c7::event {


using c7::typefunc::is_buffer_type_v;


class iovec_proxy {
private:
    ::iovec& iov_;
    using len_t = decltype(std::declval<::iovec>().iov_len);

    class iov_len_t {
    public:
	explicit iov_len_t(::iovec &iov): iov_(iov) {}
	iov_len_t& operator=(len_t n) { iov_.iov_len = n; return *this; }
	operator len_t() { return iov_.iov_len; }
	template <typename T> bool operator==(T v) const { return iov_.iov_len == v; }
	template <typename T> bool operator!=(T v) const { return !(*this == v); }
	auto print_as() const { return iov_.iov_len; }

    private:
	::iovec &iov_;
    };

    class iov_base_t {
    public:
	explicit iov_base_t(::iovec &iov): iov_(iov) {}

	// operator=(nullptr)

	iov_base_t& operator=(std::nullptr_t) {
	    debug_("(std::nullptr_t)");
	    iov_.iov_base = nullptr;
	    iov_.iov_len  = 0;
	    return *this;
	}

	// operator=(*), operator=(const*)

	iov_base_t& operator=(void *p) {
	    debug_("(void *)");
	    iov_.iov_base = p;
	    return *this;
	}
	iov_base_t& operator=(const void *p) {
	    debug_("(const void *)");
	    iov_.iov_base = const_cast<void*>(p);
	    return *this;
	}

	iov_base_t& operator=(char *s) {
	    debug_("(char *)");
	    iov_.iov_base = s;
	    iov_.iov_len  = std::strlen(s) + 1;
	    return *this;
	}
	iov_base_t& operator=(const char *s) {
	    debug_("(const char *)");
	    iov_.iov_base = const_cast<char*>(s);
	    iov_.iov_len  = std::strlen(s) + 1;
	    return *this;
	}

	iov_base_t& operator=(std::string *s) {
	    debug_("(std::string*)");
	    iov_.iov_base = const_cast<char*>(s->data());
	    iov_.iov_len  = s->size() + 1;
	    return *this;
	}
	iov_base_t& operator=(const std::string *s) {
	    debug_("(const std::string*)");
	    iov_.iov_base = const_cast<char*>(s->data());
	    iov_.iov_len  = s->size() + 1;
	    return *this;
	}

	iov_base_t& operator=(std::string_view *s) {
	    debug_("(std::string_view*)");
	    iov_.iov_base = const_cast<char*>(s->data());
	    iov_.iov_len  = s->size() + 1;
	    return *this;
	}
	iov_base_t& operator=(const std::string_view *s) {
	    debug_("(const std::string_view*)");
	    iov_.iov_base = const_cast<char*>(s->data());
	    iov_.iov_len  = s->size() + 1;
	    return *this;
	}

	template <typename T>
	iov_base_t& operator=(T *v) {
	    if constexpr (is_buffer_type_v<T>) {
		debug_("(is_buffer_type_v*)");
		using d_type = c7::typefunc::remove_cref_t<decltype(*v->data())>;
		iov_.iov_base = const_cast<d_type*>(v->data());
		iov_.iov_len  = v->size() * sizeof((*v)[0]);
	    } else {
		debug_("(T *)");
		using d_type = std::remove_const_t<T*>;
		iov_.iov_base = const_cast<d_type>(v);
		iov_.iov_len  = sizeof(T);
	    }
	    return *this;
	}
	template <typename T>
	iov_base_t& operator=(const T *v) {
	    debug_("(const T*) ->");
	    return (*this = const_cast<T*>(v));
	}

	// operator=(const&)

	template <typename T, typename = std::enable_if_t<is_buffer_type_v<T>>>
	iov_base_t& operator=(const T& v) {
	    debug_("(const is_buffer_type_v&) ->");
	    return (*this = &v);
	}

	// operator==

	bool operator==(const void *p) const {
	    return iov_.iov_base == p;
	}
	bool operator!=(const void *p) const {
	    return !operator==(p);
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

    c7::result<> size_error(size_t elm_size, size_t expect_min, size_t expect_max) const;

public:
    iov_len_t iov_len;
    iov_base_t iov_base;

    iovec_proxy(const iovec_proxy& iov):
	iov_(iov.iov_), iov_len(iov_), iov_base(iov_) {}

    explicit iovec_proxy(const ::iovec& iov):
	iov_(const_cast<::iovec&>(iov)), iov_len(iov_), iov_base(iov_) {}

    iovec_proxy& operator=(const iovec_proxy& iov) {
	if (this != &iov) {
	    iov_ = iov.iov_;
	}
	return *this;
    }

    iovec_proxy& operator=(const ::iovec& iov) {
	iov_ = iov;
	return *this;
    }

    ::iovec* operator&() { return &iov_; }

    template <typename T>
    c7::result<> strict_ptr(T*& p) const {
	p = static_cast<T*>(iov_.iov_base);
	if (iov_.iov_len == sizeof(T)) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T), 1, 1);
	}
    }
    template <typename T>
    c7::result<> strict_ptr_nullable(T*& p) const {
	p = static_cast<T*>(iov_.iov_base);
	if (iov_.iov_len == sizeof(T) || p == nullptr) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T), 0, 1);
	}
    }
    template <typename T>
    c7::result<> strict_ptr(T*& p, size_t& n) const {
	p = static_cast<T*>(iov_.iov_base);
	n = iov_.iov_len / sizeof(T);
	if (n > 0 && (iov_.iov_len % sizeof(T)) == 0) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T), 1, -1UL);
	}
    }
    template <typename T>
    c7::result<> strict_ptr_nullable(T*& p, size_t& n) const {
	p = static_cast<T*>(iov_.iov_base);
	n = iov_.iov_len / sizeof(T);
	if (n >= 0 && (iov_.iov_len % sizeof(T)) == 0) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T), 0, -1UL);
	}
    }
    template <typename T>
    c7::result<> strict_ptr_just(T*& p, size_t n) const {
	p = static_cast<T*>(iov_.iov_base);
	if ((iov_.iov_len / sizeof(T)) == n) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T), n, n);
	}
    }
    template <typename T>
    c7::result<> relaxed_ptr(T*& p) const {
	p = static_cast<T*>(iov_.iov_base);
	if (iov_.iov_len >= sizeof(T)) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T), 1, -1UL);
	}
    }
    template <typename T>
    c7::result<> relaxed_ptr_nullable(T*& p) const {
	p = static_cast<T*>(iov_.iov_base);
	if (iov_.iov_len >= sizeof(T) || p == nullptr) {
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T), 0, -1UL);
	}
    }

    template <typename T>
    c7::result<> strict_slice(c7::slice<T>& s, size_t n_min=0, size_t n_max=-1UL) const {
	auto p = static_cast<T*>(iov_.iov_base);
	auto n = iov_.iov_len / sizeof(T);
	if (n_min <= n && n <= n_max && (iov_.iov_len % sizeof(T)) == 0) {
	    s = c7::make_slice(p, n);
	    return c7result_ok();
	} else {
	    return size_error(sizeof(T), n_min, n_max);
	}
    }

    c7::result<> string_view(std::string_view& s, size_t n_min=0, size_t n_max=-1UL) const {
	auto p = static_cast<char*>(iov_.iov_base);
	auto n = iov_.iov_len;
	if (n > 0 && p[n-1] == 0) {
	    n--;
	}
	if (n_min <= n && n <= n_max) {
	    s = std::string_view{p, n};
	    return c7result_ok();
	} else {
	    return size_error(sizeof(char), n_min, n_max);
	}
    }
};


} // namespace c7::event


#endif // c7event/iovec_proxy.hpp
