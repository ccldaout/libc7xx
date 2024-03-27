/*
 * c7nseq/generator.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1sOpE7FtN5s5dtPNiGcSfTYbTDG-0lxE2PZb47yksa90/edit?usp=sharing
 */
#ifndef C7_NSEQ_GENERATOR_HPP_LOADED_
#define C7_NSEQ_GENERATOR_HPP_LOADED_


#include <c7coroutine.hpp>
#include <c7nseq/_cmn.hpp>
#include <c7nseq/empty.hpp>


namespace c7::nseq {


// generator

struct co_iter_end {};


template <typename T>
class co_output {
private:
    std::vector<T>& data_;
    c7::coroutine& co_;

public:
    co_output(std::vector<T>& data, c7::coroutine& co):
	data_(data), co_(co) {
    }

    auto& operator<<(const T& data) {
	if (data_.size() == data_.capacity()) {
	    co_.yield();
	    data_.clear();
	}
	data_.push_back(data);
	return *this;
    }

    auto& operator<<(T&& data) {
	if (data_.size() == data_.capacity()) {
	    co_.yield();
	    data_.clear();
	}
	data_.push_back(std::move(data));
	return *this;
    }

    template <typename Seq,
	      typename = std::enable_if_t<c7::typefunc::is_sequence_of_v<Seq, T>>>
    auto& operator<<(Seq&& seq) {
	for (auto&& v: seq) {
	    this->operator<<(std::forward<decltype(v)>(v));
	}
	return *this;
    }

    void flush() {
	if (data_.size() != 0) {
	    co_.yield();
	    data_.clear();
	}
    }
};


template <typename Seq, typename Output, typename Generator>
class co_iter {
private:
    struct context {
	Seq& seq_;
	Generator func_;
	c7::coroutine co_;
	std::vector<std::remove_reference_t<Output>> data_;
	bool end_;

	context(Seq& seq, Generator func, size_t buffer_size):
	    seq_(seq), func_(func), co_(1024), end_(false) {
	    data_.reserve(buffer_size);
	    co_.target([this](){
			   c7::coroutine& from = *(coroutine::self()->from());
			   co_output<value_type> out(data_, from);
			   if constexpr (std::is_invocable_v<Generator, Seq&, decltype(out)&>) {
			       func_(seq_, out);
			   } else {
			       func_(out);
			   }
			   out.flush();
			   end_ = true;
			   for (;;) {
			       from.yield();
			   }
		       });
	    co_.yield();
	}
    };

    std::shared_ptr<context> ctx_;	// for copyable
    size_t idx_ = 0;

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<Output>;
    using pointer		= value_type*;
    using reference		= value_type&;

    co_iter() = default;
    co_iter(const co_iter&) = default;
    co_iter& operator=(const co_iter&) = default;
    co_iter(co_iter&&) = default;
    co_iter& operator=(co_iter&&) = default;

    co_iter(Seq& seq, Generator func, size_t buffer_size):
	ctx_(std::make_shared<context>(seq, func, buffer_size)) {
    }

    bool operator==(const co_iter& o) const {
	return ctx_ == o.ctx_;
    }

    bool operator!=(const co_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const co_iter_end&) const {
	return ctx_->end_;
    }

    bool operator!=(const co_iter_end& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	if (idx_ == ctx_->data_.size() - 1) {
	    if (!ctx_->end_) {
		ctx_->co_.yield();
		idx_ = 0;
	    }
	} else {
	    idx_++;
	}
	return *this;
    }

    decltype(auto) operator*() {
	return std::move(ctx_->data_[idx_]);
    }
};


template <typename Seq, typename Output, typename Generator>
class co_seq {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    Generator func_;
    size_t buffer_size_;

public:
    co_seq(Seq seq, Generator func, size_t buffer_size):
	seq_(std::forward<Seq>(seq)), func_(func), buffer_size_(buffer_size) {
    }

    co_seq(const co_seq&) = delete;
    co_seq& operator=(const co_seq&) = delete;
    co_seq(co_seq&&) = default;
    co_seq& operator=(co_seq&&) = delete;

    auto begin() {
	return co_iter<hold_type, Output, Generator>(seq_, func_, buffer_size_);
    }

    auto end() {
	return co_iter_end{};
    }

    auto begin() const {
	return const_cast<co_seq<Seq, Output, Generator>*>(this)->begin();
    }

    auto end() const {
	return co_iter_end{};
    }
};


template <typename Output, typename Generator>
class co_builder {
public:
    explicit co_builder(Generator func, size_t buffer_size):
	func_(func), buffer_size_(buffer_size) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return co_seq<decltype(seq), Output, Generator>
	    (std::forward<Seq>(seq), func_, buffer_size_);
    }

    auto operator()() {
	empty_seq<> seq;
	return co_seq<decltype(seq), Output, Generator>
	    (seq, func_, buffer_size_);
    }

private:
    Generator func_;
    size_t buffer_size_;
};


template <typename Output, typename Generator>
auto generator(Generator func, size_t buffer_size=1024)
{
    return co_builder<Output, Generator>(func, buffer_size);
};


// for compatibility
template <typename Output, typename Generator>
using generator_seq = co_builder<Output, Generator>;


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <typename Seq, typename Output, typename Generator>
struct format_ident<c7::nseq::co_seq<Seq, Output, Generator>> {
    static constexpr const char *name = "generator";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/generator.hpp
