/*
 * c7event/msgbuf.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_MSGBUF_HPP_LOADED__
#define C7_EVENT_MSGBUF_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/portgroup.hpp>


namespace c7 {
namespace event {


/*----------------------------------------------------------------------------
                   message buffer (default implementation)
----------------------------------------------------------------------------*/

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
	iov_base_t& operator=(void *p) { iov_->iov_base = p; return *this; }
	iov_base_t& operator=(const void *p) { iov_->iov_base = const_cast<void*>(p); return *this; }
	iov_base_t& operator=(const char *s) { iov_->iov_base = const_cast<char*>(s); iov_->iov_len = strlen(s) + 1; return *this; }
	iov_base_t& operator=(const std::string& s) { return operator=(s.c_str()); }
	template <typename T> iov_base_t& operator=(T *p) { iov_->iov_base = p; iov_->iov_len = sizeof(T); return *this; }
	template <typename T> iov_base_t& operator=(const T *p) { return operator=(const_cast<T*>(p)); }
	template <typename T> iov_base_t& operator=(const std::vector<T>& v) {
	    iov_->iov_base = const_cast<void*>(static_cast<const void*>(v.data()));
	    iov_->iov_len = sizeof(T) * v.size();
	    return *this;
	}
	operator void* () { return iov_->iov_base; }
	template <typename T> operator T* () { return as<T>(); }
	template <typename T> T* as() { return static_cast<T*>(iov_->iov_base); }
	auto print_as() const { return iov_->iov_base; }
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
};


template <typename Header, int N>
class multipart_impl;


template <typename Header, int N>
class multipart_msgbuf {
private:
    template <typename, int>
    friend class multipart_msgbuf;

    std::unique_ptr<multipart_impl<Header, N>> impl_;

public:
    static constexpr int n_part = N;
    Header& header;

    ~multipart_msgbuf();
    multipart_msgbuf();
    multipart_msgbuf(const multipart_msgbuf& o) = delete;
    multipart_msgbuf(multipart_msgbuf&& o);
    multipart_msgbuf& operator=(const multipart_msgbuf& o) = delete;
    multipart_msgbuf& operator=(multipart_msgbuf&& o);

    void clear();
    multipart_msgbuf deep_copy() const;
    multipart_msgbuf& deep_copy_from(const multipart_msgbuf& src);

    template <typename H2> multipart_msgbuf<H2, N> deep_copy_iov() const;
    template <typename H2> multipart_msgbuf& deep_copy_iov_from(const multipart_msgbuf<H2, N>& src);
    template <typename H2> multipart_msgbuf<H2, N> move_iov() const;
    template <typename H2> multipart_msgbuf& move_iov_from(multipart_msgbuf<H2, N>&& src);
    template <typename H2> multipart_msgbuf& borrow_iov_from(const multipart_msgbuf<H2, N>& src);

    template <typename Port> io_result recv(Port& port);
    template <typename Port> io_result send(Port& port);
    template <typename Port> result<> send(portgroup<Port>& ports);
    void dump() const;

    iovec_proxy operator[](int n);
    const iovec_proxy operator[](int n) const;
};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::event
} // namespace c7


#endif // c7event/msgbuf.hpp
