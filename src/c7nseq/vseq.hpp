/*
 * c7nseq/vseq.hpp
 *
 * Copyright (c) 2025 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_VSEQ_HPP_LOADED_
#define C7_NSEQ_VSEQ_HPP_LOADED_


#include <atomic>
#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


class vseq_iter_end {};


template <typename T>
class vseq_iter_concept {
private:
    const uint64_t seq_id_;

protected:
    bool same_seq(const vseq_iter_concept& o) const {
	return seq_id_ == o.seq_id_;
    }

public:
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<T>;
    using pointer		= value_type*;
    using reference		= value_type&;

    virtual ~vseq_iter_concept() {}

    explicit vseq_iter_concept(uint64_t seq_id): seq_id_(seq_id) {}

    virtual bool operator==(const vseq_iter_concept& o) const = 0;

    virtual bool operator==(const vseq_iter_end&) const = 0;

    virtual vseq_iter_concept& operator++() = 0;

    virtual T operator*() = 0;

    virtual const T operator*() const = 0;

    bool operator!=(const vseq_iter_concept& o) const {
	return !(*this == o);
    }

    bool operator!=(const vseq_iter_end& o) const {
	return !(*this == o);
    }
};


template <typename T, typename Iter, typename Iterend>
class vseq_iter_impl: public vseq_iter_concept<T> {
private:
    Iter it_;
    Iterend itend_;

public:
    vseq_iter_impl(uint64_t seq_id, Iter it, Iterend itend):
	vseq_iter_concept<T>(seq_id),
	it_(it), itend_(itend) {
    }

    bool operator==(const vseq_iter_concept<T>& o) const override {
	return (this->same_seq(o) &&
		it_ == static_cast<const vseq_iter_impl<T, Iter, Iterend>&>(o).it_);
    }

    bool operator==(const vseq_iter_end&) const override {
	return (it_ == itend_);
    }

    vseq_iter_impl& operator++() override {
	++it_;
	return *this;
    }

    T operator*() override {
	return *it_;
    }

    const T operator*() const override {
	return *it_;
    }
};


template <typename T>
class vseq_iter {
private:
    std::shared_ptr<vseq_iter_concept<T>> itp_;

public:
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<T>;
    using pointer		= value_type*;
    using reference		= value_type&;

    vseq_iter() = default;
    vseq_iter(const vseq_iter&) = default;
    vseq_iter(vseq_iter&&) = default;
    vseq_iter& operator=(const vseq_iter&) = default;
    vseq_iter& operator=(vseq_iter&&) = default;

    template <typename Iter, typename Iterend>
    vseq_iter(uint64_t seq_id, Iter it, Iterend itend):
	itp_(new vseq_iter_impl<T, Iter, Iterend>(seq_id, it, itend)) {
    }

    bool operator==(const vseq_iter& o) const {
	return (*itp_ == *o.itp_);
    }

    bool operator==(const vseq_iter_end& o) const {
	return (*itp_ == o);
    }

    bool operator!=(const vseq_iter& o) const {
	return !(*this == o);
    }

    bool operator!=(const vseq_iter_end& o) const {
	return !(*this == o);
    }

    vseq_iter& operator++() {
	++(*itp_);
	return *this;
    }

    decltype(auto) operator*() {
	return **itp_;
    }

    decltype(auto) operator*() const {
	return **itp_;
    }
};


template <typename Value>
class vseq_concept {
public:
    virtual ~vseq_concept() {}
    virtual vseq_iter<Value> begin() = 0;
    virtual vseq_iter<Value> begin() const = 0;
};


template <typename Seq, typename Value>
class vseq_impl: public vseq_concept<Value> {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    const uint64_t seq_id_;

    static uint64_t get_id() {
	static std::atomic<uint64_t> seq_id;
	return ++seq_id;
    }

public:
    explicit vseq_impl(Seq seq):
	seq_(std::forward<Seq>(seq)), seq_id_(get_id()) {
    }

    vseq_iter<Value> begin() override {
	using std::begin;
	using std::end;
	return vseq_iter<Value>{seq_id_, begin(seq_), end(seq_)};
    }

    vseq_iter<Value> begin() const override {
	using std::begin;
	using std::end;
	return vseq_iter<Value>{seq_id_, begin(seq_), end(seq_)};
    }
};


template <typename Value>
class vseq {
private:
    std::unique_ptr<vseq_concept<Value>> vsp_;

public:
    template <typename Seq>
    vseq(Seq&& seq):
	vsp_(new vseq_impl<Seq, Value>(std::forward<Seq>(seq))) {
    }

    vseq_iter<Value> begin() {
	return vsp_->begin();
    }

    vseq_iter<Value> begin() const {
	return vsp_->begin();
    }

    vseq_iter_end end()	const {
	return vseq_iter_end{};
    }
};


template <typename Seq>
auto to_vseq(Seq&& seq)
{
    using Value = std::remove_reference_t<decltype(*begin(std::declval<Seq>()))>;
    return vseq<Value>(std::forward<Seq>(seq));
}


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <typename... Seqs>
struct format_ident<c7::nseq::vseq<Seqs...>> {
    static constexpr const char *name = "vseq";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/vseq.hpp
