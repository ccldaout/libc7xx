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


#include <c7generator_r2.hpp>
#include <c7nseq/_cmn.hpp>
#include <c7nseq/empty.hpp>
#include <c7utils.hpp>


namespace c7::nseq {


// generator

static inline constexpr size_t C7_NSEQ_GEN_STACKSIZE = 8192;


template <typename Output, size_t StackSize = C7_NSEQ_GEN_STACKSIZE>
using co_output = typename c7::r2::generator<Output, StackSize>::gen_output;


template <typename Output, size_t StackSize = C7_NSEQ_GEN_STACKSIZE>
class co_seq {
private:

    using gen_type = c7::r2::generator<Output, StackSize>;
    std::shared_ptr<gen_type> gen_;

public:
    template <typename Seq, typename Generator>
    co_seq(Seq&& seq, Generator&& func, size_t buffer_size):
	gen_(std::make_shared<gen_type>(buffer_size)) {
	if constexpr (std::is_lvalue_reference_v<Seq>) {
	    gen_->start(
		[&seq,
		 func=std::forward<Generator>(func)] (auto& out) mutable {
		    func(seq, out);
		});
	} else {
	    gen_->start(
		[seq=c7::movable_capture(seq),
		 func=std::forward<Generator>(func)] (auto& out) mutable {
		    auto s = seq.unwrap();
		    func(s, out);
		});
	}
    }

    template <typename Generator>
    co_seq(Generator&& func, size_t buffer_size):
	gen_(std::make_shared<gen_type>(buffer_size)) {
	gen_->start(
	    [func=std::forward<Generator>(func)] (auto& out) {
		func(out);
	    });
    }

    co_seq(const co_seq&) = default;
    co_seq& operator=(const co_seq&) = default;
    co_seq(co_seq&&) = default;
    co_seq& operator=(co_seq&&) = delete;

    auto begin() {
	return gen_->begin();
    }

    auto end() {
	return gen_->end();
    }

    auto begin() const {
	return gen_->begin();
    }

    auto end() const {
	return gen_->end();
    }
};


template <typename Output, typename Generator,
	  size_t StackSize = C7_NSEQ_GEN_STACKSIZE>
class co_builder {
public:
    explicit co_builder(Generator func, size_t buffer_size):
	func_(func), buffer_size_(buffer_size) {}

    template <typename Seq>
    auto operator()(Seq&& seq) {
	return co_seq<Output, StackSize>
	    (std::forward<Seq>(seq), func_, buffer_size_);
    }

    auto operator()() {
	return co_seq<Output, StackSize>
	    (func_, buffer_size_);
    }

private:
    Generator func_;
    size_t buffer_size_;
};


// Generator
//
//   void func(Sequence& source_seq, c7::nseq::co_output<Output>& out);
//   void func(                      c7::nseq::co_output<Output>& out);  [head of pipeline]
//
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
template <typename Output, size_t StackSize>
struct format_ident<c7::nseq::co_seq<Output, StackSize>> {
    static constexpr const char *name = "generator";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/generator.hpp
