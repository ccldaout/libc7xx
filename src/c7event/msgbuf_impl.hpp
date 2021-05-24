/*
 * c7event/msgbuf_impl.hpp
 *
 * Copyright (c) 2021 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_EVENT_MSGBUF_IMPL_HPP_LOADED__
#define C7_EVENT_MSGBUF_IMPL_HPP_LOADED__
#include <c7common.hpp>


#include <c7app.hpp>
#include <c7event/msgbuf.hpp>
#include <c7utils.hpp>
#include <cstring>


namespace c7::event {


/*----------------------------------------------------------------------------
                                multipart_impl
----------------------------------------------------------------------------*/

template <typename Header, int N>
class multipart_impl {
public:
    static constexpr int n_part = N;
    Header& header { header_.header };

    ~multipart_impl() = default;
    multipart_impl();
    multipart_impl(const multipart_impl& o) = delete;
    multipart_impl(multipart_impl&& o) = delete;
    multipart_impl& operator=(const multipart_impl& o) = delete;
    multipart_impl& operator=(multipart_impl&& o) = delete;

    void clear();
    multipart_impl deep_copy() const;
    multipart_impl& deep_copy_from(const multipart_impl& src);

    template <typename H2> multipart_impl<H2, N> deep_copy_iov() const;
    template <typename H2> multipart_impl& deep_copy_iov_from(const multipart_impl<H2, N>& src);
    template <typename H2> multipart_impl<H2, N> move_iov() const;
    template <typename H2> multipart_impl& move_iov_from(multipart_impl<H2, N>&& src);
    template <typename H2> multipart_impl& borrow_iov_from(const multipart_impl<H2, N>& src);

    template <typename Port> io_result recv(Port& port);
    template <typename Port> io_result send(Port& port);
    template <typename Port> result<> send(portgroup<Port>& ports);
    void dump() const;

    iovec_proxy operator[](int n) { return iovec_proxy{&iov_[n]}; }
    const iovec_proxy operator[](int n) const { return iovec_proxy{&iov_[n]}; }

private:
    template <typename, int>
    friend class multipart_impl;

    static constexpr int part_align_ = 8;
    static constexpr int whole_align_ = 8192;

    struct internal_header {
	Header header;
	int32_t size[N];
    };

    internal_header header_;
    ::iovec iov_[N + 1];
    c7::storage storage_ {whole_align_};

    void setup_iov_0();
    void setup_iov_len(const internal_header&, ::iovec (&)[N+1]);
    result<> setup_iov_base(c7::storage&, ::iovec (&)[N+1]);
    template <typename H2> void copy_contents(const multipart_impl<H2, N>& o);
    template <typename H2> void move_contents(multipart_impl<H2, N>&& o);

    static_assert(sizeof(Header)+sizeof(int32_t[N]) == sizeof(internal_header),
		  "Combination Header and N has whole.");
};


template <typename Header, int N>
multipart_impl<Header, N>::multipart_impl()
{
    std::memset(&header_, 0, sizeof(header_));
    std::memset(iov_, 0, sizeof(iov_));
    setup_iov_0();
}

template <typename Header, int N>
void
multipart_impl<Header, N>::clear()
{
    std::memset(&iov_[1], 0, sizeof(iov_) - sizeof(iov_[0]));
}

template <typename Header, int N>
auto
multipart_impl<Header, N>::deep_copy() const -> multipart_impl
{
    multipart_impl b;
    b.copy_contents(*this);
    return b;
}

template <typename Header, int N>
auto
multipart_impl<Header, N>::deep_copy_from(const multipart_impl& o) -> multipart_impl&
{
    if (&o != this) {
	copy_contents(o);
    }
    return *this;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_impl<Header, N>::deep_copy_iov() const -> multipart_impl<H2, N>
{
    multipart_impl<H2, N> b;
    b.copy_contents(*this);
    return b;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_impl<Header, N>::deep_copy_iov_from(const multipart_impl<H2, N>& o) -> multipart_impl&
{
    if (static_cast<const void*>(&o) != this) {
	copy_contents(o);
    }
    return *this;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_impl<Header, N>::move_iov() const -> multipart_impl<H2, N>
{
    multipart_impl<H2, N> b;
    b.move_contents(*this);
    return b;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_impl<Header, N>::move_iov_from(multipart_impl<H2, N>&& o) -> multipart_impl&
{
    if (static_cast<void*>(&o) != this) {
	move_contents(o);
    }
    return *this;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_impl<Header, N>::borrow_iov_from(const multipart_impl<H2, N>& o) -> multipart_impl&
{
    if (static_cast<const void*>(&o) != this) {
	std::memcpy(iov_, o.iov_, sizeof(iov_));
	setup_iov_0();
    }
    return *this;
}

template <typename Header, int N>
template <typename Port>
io_result
multipart_impl<Header, N>::recv(Port& port)
{
    auto iores = port.read_n(&header_, sizeof(header_));
    if (!iores) {
	return iores;
    }
    if (port.is_different_endian()) {
	for (auto& v: header_.size) {
	    c7::endian::reverse(v);
	}
    }
    setup_iov_len(header_, iov_);
    if (auto res = setup_iov_base(storage_, iov_); !res) {
	return io_result::error(port, std::move(res));
    }
    for (int i = 1; i <= N; i++) {
	if (iov_[i].iov_len > 0) {
	    iores = port.read_n(iov_[i].iov_base, iov_[i].iov_len);
	    if (!iores) {
		return iores;
	    }
	}
    }
    return io_result::ok();
}

template <typename Header, int N>
template <typename Port>
io_result
multipart_impl<Header, N>::send(Port& port)
{
    if (port.is_different_endian()) {
	for (int i = 1; i <= N; i++) {
	    // BUG: header_.size[i] = reverse_to(iov_[i].iov_len);
	    //      because sizeof(iov_len) > sizeof(header_.size[i])
	    header_.size[i-1] = iov_[i].iov_len;
	    c7::endian::reverse(header_.size[i-1]);
	}
    } else {
	for (int i = 1; i <= N; i++) {
	    header_.size[i-1] = iov_[i].iov_len;
	}
    }
	
    // [CAUTION] port.write_v change contents of iov and ioc
    int ioc = N + 1;
    ::iovec iov[N + 1];
    ::iovec *iovp = iov;
    std::memcpy(iov, iov_, sizeof(iov));
    auto res = port.write_v(iovp, ioc);

    return std::move(res);
}

template <typename Header, int N>
template <typename Port>
result<>
multipart_impl<Header, N>::send(portgroup<Port>& ports)
{
    ports.clear_errors();
    for (auto pp: ports) {
	if (pp != nullptr) {
	    auto& port = *pp;
	    if (auto io_res = send(port); !io_res) {
		ports.add_error(port, std::move(io_res));
	    }
	}
    }
    if (ports) {
	return c7result_ok();
    } else {
	return c7result_err("Error on %{} port(s)", ports.errors().size());
    }
}

template <typename Header, int N>
template <typename H2>
void
multipart_impl<Header, N>::copy_contents(const multipart_impl<H2, N>& o)
{
    if (std::is_same_v<Header, H2>) {
	std::memcpy(&header_, &o.header_, sizeof(header_));
    }
    std::memcpy(iov_, o.iov_, sizeof(iov_));
    setup_iov_0();

    if (auto res = setup_iov_base(storage_, iov_); !res) {
	throw std::runtime_error(c7::format("%{}", res));
    }
    for (int i = 1; i <= N; i++) {
	std::memcpy(iov_[i].iov_base, o.iov_[i].iov_base, iov_[i].iov_len);
    }
}

template <typename Header, int N>
template <typename H2>
void
multipart_impl<Header, N>::move_contents(multipart_impl<H2, N>&& o)
{
    if (std::is_same_v<Header, H2>) {
	std::memcpy(&header_, &o.header_, sizeof(header_));
	std::memset(&o.header_, 0, sizeof(o.header_));
    }
    std::memcpy(iov_, o.iov_, sizeof(iov_));
    std::memset(o.iov_, 0, sizeof(o.iov_));
    setup_iov_0();
    o.setup_iov_0();

    storage_ = std::move(o.storage_);
}

template <typename Header, int N>
void
multipart_impl<Header, N>::setup_iov_0()
{
    iov_[0].iov_base = &header_;
    iov_[0].iov_len  = sizeof(header_);
}

template <typename Header, int N>
void
multipart_impl<Header, N>::setup_iov_len(const internal_header& h, ::iovec (&iov)[N+1])
{
    for (int i = 1; i <= N; i++) {
	iov[i].iov_len = h.size[i-1];
    }
}

template <typename Header, int N>
result<>
multipart_impl<Header, N>::setup_iov_base(c7::storage& storage, ::iovec (&iov)[N+1])
{
    size_t new_size = 0;
    for (int i = 1; i <= N; i++) {
	new_size += c7_align(iov[i].iov_len, part_align_);
    }
    if (auto res = storage.reserve(new_size); !res) {
	return c7result_err(errno, "cannot extend storage: %{}", new_size);
    }
    char *sp = storage;
    for (int i = 1; i <= N; i++) {
	if (auto n = iov[i].iov_len; n > 0) {
	    iov[i].iov_base = sp;
	    sp += c7_align(n, part_align_);
	} else {
	    iov[i].iov_base = nullptr;
	}
    }
    return c7result_ok();
}

template <typename Header, int N>
void
multipart_impl<Header, N>::dump() const
{
    std::string data;
    if constexpr (!std::is_same_v<typename c7::formatter_tag<Header>::type, c7::formatter_error_tag>) {
	c7::p_("header: %{}", header);
    }

    for (int i = 1; i <= N; i++) {
	data.clear();
	if (char *p = static_cast<char*>(iov_[i].iov_base); p) {
	    for (auto m = std::min<size_t>(16, iov_[i].iov_len); m > 0; m--, p++) {
		if (std::isgraph(*p)) {
		    data += "  ";
		    data += *p;
		} else {
		    data += c7::format(" %{02x}", static_cast<int>(*p));
		}
	    }
	}
	c7::p_(" iov[%{2}] len:%{}, base:%{} '%{}'",
	       i, iov_[i].iov_len, iov_[i].iov_base, data);
    }
}


/*----------------------------------------------------------------------------
                               multipart_msgbuf
----------------------------------------------------------------------------*/

template <typename Header, int N>
multipart_msgbuf<Header, N>::~multipart_msgbuf() = default;

template <typename Header, int N>
multipart_msgbuf<Header, N>::multipart_msgbuf():
    impl_(new multipart_impl<Header, N>()), header(impl_->header)
{
}

template <typename Header, int N>
multipart_msgbuf<Header, N>::multipart_msgbuf(multipart_msgbuf&& o):
    impl_(std::move(o.impl_)), header(impl_->header)
{
}

template <typename Header, int N>
auto
multipart_msgbuf<Header, N>::operator=(multipart_msgbuf&& o) -> multipart_msgbuf&
{
    if (&o != this) {
	impl_ = std::move(o.impl_);
    }
    return *this;
}

template <typename Header, int N>
void
multipart_msgbuf<Header, N>::change_impl(std::unique_ptr<multipart_impl<Header, N>> impl)
{
    impl_ = std::move(impl_);
}

template <typename Header, int N>
void
multipart_msgbuf<Header, N>::clear()
{
    impl_->clear();
}

template <typename Header, int N>
auto
multipart_msgbuf<Header, N>::deep_copy() const -> multipart_msgbuf
{
    multipart_msgbuf b;
    b.deep_copy_from(*this);
    return b;
}

template <typename Header, int N>
auto
multipart_msgbuf<Header, N>::deep_copy_from(const multipart_msgbuf& o) -> multipart_msgbuf&
{
    if (&o != this) {
	impl_->deep_copy_from(*o.impl_);
    }
    return *this;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_msgbuf<Header, N>::deep_copy_iov() const -> multipart_msgbuf<H2, N>
{
    multipart_msgbuf<H2, N> b;
    b.deep_copy_iov_from(*this);
    return b;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_msgbuf<Header, N>::deep_copy_iov_from(const multipart_msgbuf<H2, N>& o) -> multipart_msgbuf&
{
    if (static_cast<const void*>(&o) != this) {
	impl_->deep_copy_iov_from(*o.impl_);
    }
    return *this;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_msgbuf<Header, N>::move_iov() const -> multipart_msgbuf<H2, N>
{
    multipart_msgbuf b;
    b.move_iov_from(*this);
    return b;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_msgbuf<Header, N>::move_iov_from(multipart_msgbuf<H2, N>&& o) -> multipart_msgbuf&
{
    if (static_cast<void*>(&o) != this) {
	impl_->move_iov_from(*o.impl_);
    }
    return *this;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_msgbuf<Header, N>::borrow_iov_from(const multipart_msgbuf<H2, N>& o) -> multipart_msgbuf&
{
    if (static_cast<const void*>(&o) != this) {
	impl_->borrow_iov_from(*o.impl_);
    }
    return *this;
}

template <typename Header, int N>
template <typename Port>
io_result
multipart_msgbuf<Header, N>::recv(Port& port)
{
    return impl_->recv(port);
}

template <typename Header, int N>
template <typename Port>
io_result
multipart_msgbuf<Header, N>::send(Port& port)
{
    return impl_->send(port);
}

template <typename Header, int N>
template <typename Port>
result<>
multipart_msgbuf<Header, N>::send(portgroup<Port>& ports)
{
    return impl_->send(ports);
}

template <typename Header, int N>
iovec_proxy multipart_msgbuf<Header, N>::operator[](int n)
{
    return (*impl_)[n];
}

template <typename Header, int N>
const iovec_proxy multipart_msgbuf<Header, N>::operator[](int n) const
{
    return (*impl_)[n];
}

template <typename Header, int N>
void
multipart_msgbuf<Header, N>::dump() const
{
    if (impl_ == nullptr) {
	c7::p_("already broken (maybe moved)");
    } else {
	impl_->dump();
    }
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::event


#endif // c7event/msgbuf_impl.hpp
