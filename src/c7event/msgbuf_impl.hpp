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
#include <cstring>


namespace c7::event {


template <typename Header, int N>
multipart_msgbuf<Header, N>::multipart_msgbuf():
    header(Header()), iov_{}
{
}

template <typename Header, int N>
multipart_msgbuf<Header, N>::multipart_msgbuf(multipart_msgbuf&& o):
    header(o.header), storage_(std::move(o.storage_))
{
    std::memcpy(iov_, o.iov_, sizeof(iov_));
}

template <typename Header, int N>
auto
multipart_msgbuf<Header, N>::operator=(multipart_msgbuf&& o) -> multipart_msgbuf&
{
    header = o.header;
    std::memcpy(iov_, o.iov_, sizeof(iov_));
    storage_ = std::move(o.storage_);
    return *this;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_msgbuf<Header, N>::operator=(multipart_msgbuf<H2, N>&& o) -> multipart_msgbuf&
{
    if (static_cast<void*>(&o) != this) {
	move_contents(o);
    }
    return *this;
}

template <typename Header, int N>
void
multipart_msgbuf<Header, N>::clear()
{
    std::memset(&iov_[1], 0, sizeof(iov_) - sizeof(iov_[0]));
}

template <typename Header, int N>
auto
multipart_msgbuf<Header, N>::deep_copy() const -> multipart_msgbuf
{
    multipart_msgbuf b;
    b.copy_contents(*this);
    return b;
}

template <typename Header, int N>
auto
multipart_msgbuf<Header, N>::deep_copy_from(const multipart_msgbuf& o) -> multipart_msgbuf&
{
    if (&o != this) {
	copy_contents(o);
    }
    return *this;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_msgbuf<Header, N>::deep_copy_iov() const -> multipart_msgbuf<H2, N>
{
    multipart_msgbuf<H2, N> b;
    b.copy_contents(*this);
    return b;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_msgbuf<Header, N>::deep_copy_iov_from(const multipart_msgbuf<H2, N>& o) -> multipart_msgbuf&
{
    if (static_cast<const void*>(&o) != this) {
	copy_contents(o);
    }
    return *this;
}

template <typename Header, int N>
template <typename H2>
auto
multipart_msgbuf<Header, N>::borrow_iov_from(const multipart_msgbuf<H2, N>& o) -> multipart_msgbuf&
{
    if (static_cast<const void*>(&o) != this) {
	std::memcpy(iov_, o.iov_, sizeof(iov_));
    }
    return *this;
}

template <typename Header, int N>
template <typename Port>
io_result
multipart_msgbuf<Header, N>::recv(Port& port)
{
    internal_header header;

    auto iores = port.read_n(&header, sizeof(header));
    if (!iores) {
	return iores;
    }
    this->header = header.header;
    if (port.is_different_endian()) {
	for (auto& v: header.size) {
	    c7::endian::reverse(v);
	}
    }
    setup_iov_len(header, iov_);
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
multipart_msgbuf<Header, N>::send(Port& port)
{
    internal_header header;

    header.header = this->header;
    iov_[0].iov_base = &header;
    iov_[0].iov_len  = sizeof(header);

    if (port.is_different_endian()) {
	for (int i = 1; i <= N; i++) {
	    // BUG: header.size[i] = reverse_to(iov_[i].iov_len);
	    //      because sizeof(iov_len) > sizeof(header.size[i])
	    header.size[i-1] = iov_[i].iov_len;
	    c7::endian::reverse(header.size[i-1]);
	}
    } else {
	for (int i = 1; i <= N; i++) {
	    header.size[i-1] = iov_[i].iov_len;
	}
    }

    // [CAUTION] port.write_v change contents of iov and ioc
    int ioc = N + 1;
    ::iovec iov[N + 1];
    ::iovec *iovp = iov;
    std::memcpy(iov, iov_, sizeof(iov));
    auto res = port.write_v(iovp, ioc);

    return res;
}

template <typename Header, int N>
template <typename Port>
result<>
multipart_msgbuf<Header, N>::send(portgroup<Port>& ports)
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
multipart_msgbuf<Header, N>::copy_contents(const multipart_msgbuf<H2, N>& o)
{
    if (std::is_same_v<Header, H2>) {
	std::memcpy(&header, &o.header, sizeof(header));
    }
    std::memcpy(iov_, o.iov_, sizeof(iov_));

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
multipart_msgbuf<Header, N>::move_contents(multipart_msgbuf<H2, N>& o)
{
    if (std::is_same_v<Header, H2>) {
	std::memcpy(&header, &o.header, sizeof(header));
	std::memset(&o.header, 0, sizeof(o.header));
    }
    std::memcpy(iov_, o.iov_, sizeof(iov_));
    std::memset(o.iov_, 0, sizeof(o.iov_));

    storage_ = std::move(o.storage_);
}

template <typename Header, int N>
void
multipart_msgbuf<Header, N>::setup_iov_len(const internal_header& h, ::iovec (&iov)[N+1])
{
    for (int i = 1; i <= N; i++) {
	iov[i].iov_len = h.size[i-1];
    }
}

template <typename Header, int N>
result<>
multipart_msgbuf<Header, N>::setup_iov_base(c7::storage& storage, ::iovec (&iov)[N+1])
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
multipart_msgbuf<Header, N>::dump() const
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


} // namespace c7::event


#endif // c7event/msgbuf_impl.hpp
