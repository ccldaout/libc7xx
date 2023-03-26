/*
 * c7nseq/generator.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (nothing)
 */
#ifndef C7_NSEQ_GENERATOR_HPP_LOADED__
#define C7_NSEQ_GENERATOR_HPP_LOADED__


#include <c7coroutine.hpp>
#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


// generator

struct generator_iter_end {};


template <typename T>
class co_output {
private:
    T& data_;
    c7::coroutine& co_;

public:
    co_output(T& data, c7::coroutine& co): data_(data), co_(co) {
    }

    void operator<<(const T& data) {
	data_ = data;
	co_.yield();
    }

    void operator<<(T&& data) {
	data_ = std::move(data);
	co_.yield();
    }

};


template <typename Seq, typename Output, typename Generator>
class generator_iter {
private:
    struct context {
	Seq& seq_;
	Generator func_;
	c7::coroutine co_;
	std::remove_reference_t<Output> data_;
	bool end_;

	context(Seq& seq, Generator func):
	    seq_(seq), func_(func), co_(1024), end_(false) {
	    co_.target([this](){
			   c7::coroutine& from = *(coroutine::self()->from());
			   co_output<value_type> out(data_, from);
			   func_(seq_, out);
			   end_ = true;
			   for (;;) {
			       from.yield();
			   }
		       });
	    co_.yield();
	}
    };

    std::shared_ptr<context> ctx_;	// for copyable

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= std::remove_reference_t<Output>;
    using pointer		= value_type*;
    using reference		= value_type&;

    generator_iter(Seq& seq, Generator func):
	ctx_(std::make_shared<context>(seq, func)) {
    }

    bool operator==(const generator_iter& o) const {
	return ctx_ == o.ctx_;
    }

    bool operator!=(const generator_iter& o) const {
	return !(*this == o);
    }

    bool operator==(const generator_iter_end&) const {
	return ctx_->end_;
    }

    bool operator!=(const generator_iter_end& o) const {
	return !(*this == o);
    }

    auto& operator++() {
	if (!ctx_->end_) {
	    ctx_->co_.yield();
	}
	return *this;
    }

    decltype(auto) operator*() {
	return std::move(ctx_->data_);
    }
};


template <typename Seq, typename Output, typename Generator>
class generator_obj {
private:
    using hold_type =
	c7::typefunc::ifelse_t<std::is_rvalue_reference<Seq>,
			       std::remove_reference_t<Seq>,
			       Seq>;
    hold_type seq_;
    Generator func_;

public:
    generator_obj(Seq seq, Generator func):
	seq_(std::forward<Seq>(seq)), func_(func) {
    }

    generator_obj(const generator_obj&) = delete;
    generator_obj& operator=(const generator_obj&) = delete;
    generator_obj(generator_obj&&) = default;
    generator_obj& operator=(generator_obj&&) = delete;

    auto begin() {
	return generator_iter<hold_type, Output, Generator>(seq_, func_);
    }

    auto end() {
	return generator_iter_end{};
    }

    auto begin() const {
	return const_cast<generator_obj<Seq, Output, Generator>*>(this)->begin();
    }

    auto end() const {
	return generator_iter_end{};
    }
};


template <typename Output, typename Generator>
class generator_seq {
public:
    explicit generator_seq(Generator func): func_(func) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return generator_obj<decltype(seq), Output, Generator>
	    (std::forward<Seq>(seq), func_);
    }

    auto operator()() {
	std::vector<char> seq;
	return generator_obj<decltype(seq), Output, Generator>
	    (seq, func_);
    }

private:
    Generator func_;
};


template <typename Output, typename Generator>
auto generator(Generator func)
{
    return generator_seq<Output, Generator>(func);
};


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED__)
namespace c7::format_helper {
template <typename Seq, typename Output, typename Generator>
struct format_ident<c7::nseq::generator_obj<Seq, Output, Generator>> {
    static constexpr const char *name = "generator";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/generator.hpp
