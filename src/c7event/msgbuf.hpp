/*
 * c7event/msgbuf.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
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


#include <c7event/iovec_proxy.hpp>
#include <c7event/portgroup.hpp>


// message buffer (default implementation)


namespace c7::event {


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

    void change_impl(std::unique_ptr<multipart_impl<Header, N>> impl);

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


} // namespace c7::event


#endif // c7event/msgbuf.hpp
