/*
 * c7event/msgbuf.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1_2Pj_MDBpX0PwGYouK46sXM1qWyUOi8iUv1zynuXqA0/edit?usp=sharing
 */
#ifndef C7_EVENT_MSGBUF_HPP_LOADED__
#define C7_EVENT_MSGBUF_HPP_LOADED__
#include <c7common.hpp>


#include <c7event/iovec_proxy.hpp>
#include <c7event/portgroup.hpp>
#include <c7utils.hpp>			// c7::storage


// message buffer (default implementation)


namespace c7::event {


template <typename Header, int N>
class multipart_msgbuf {
public:
    static constexpr int n_part = N;
    Header header;

    ~multipart_msgbuf() = default;
    multipart_msgbuf();
    multipart_msgbuf(const multipart_msgbuf& o) = delete;
    multipart_msgbuf(multipart_msgbuf&& o);
    multipart_msgbuf& operator=(const multipart_msgbuf& o) = delete;
    multipart_msgbuf& operator=(multipart_msgbuf&& o);
    template <typename H2> multipart_msgbuf& operator=(multipart_msgbuf<H2, N>&& src);

    void clear();
    multipart_msgbuf deep_copy() const;
    multipart_msgbuf& deep_copy_from(const multipart_msgbuf& src);

    template <typename H2=Header> multipart_msgbuf<H2, N> deep_copy_iov() const;
    template <typename H2> multipart_msgbuf& deep_copy_iov_from(const multipart_msgbuf<H2, N>& src);
    template <typename H2> multipart_msgbuf& borrow_iov_from(const multipart_msgbuf<H2, N>& src);

    template <typename Port> io_result recv(Port& port);
    template <typename Port> io_result send(Port& port);
    template <typename Port> result<> send(portgroup<Port>& ports);
    void dump() const;

    iovec_proxy operator[](int n) { return iovec_proxy{iov_[n]}; }
    const iovec_proxy operator[](int n) const { return iovec_proxy{iov_[n]}; }

private:
    template <typename, int>
    friend class multipart_msgbuf;

    static constexpr int part_align_ = 8;
    static constexpr int whole_align_ = 8192;

    struct internal_header {
	Header header;
	int32_t size[N];
    };

    ::iovec iov_[N + 1];
    c7::storage storage_ {whole_align_};

    void setup_iov_len(const internal_header&, ::iovec (&)[N+1]);
    result<> setup_iov_base(c7::storage&, ::iovec (&)[N+1]);
    template <typename H2> void copy_contents(const multipart_msgbuf<H2, N>& o);
    template <typename H2> void move_contents(multipart_msgbuf<H2, N>& o);

    static_assert(sizeof(Header)+sizeof(int32_t[N]) == sizeof(internal_header),
		  "Combination Header and N has whole.");
};


} // namespace c7::event


#endif // c7event/msgbuf.hpp
