/*
 * c7generator_r2.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_GENERATOR_R2_HPP_LOADED_
#define C7_GENERATOR_R2_HPP_LOADED_
#include <c7common.hpp>


#include <c7context.hpp>
#include <c7typefunc.hpp>


namespace c7::r2 {


template <typename Output, size_t StackSize = 1024>
class generator {
public:
    class gen_output {
    public:
	template <typename Data>
	gen_output& operator<<(Data&& data) {
	    if constexpr (c7::typefunc::is_sequence_of_v<Data, Output>) {
		for (auto&& v: data) {
		    gen_->new_data(std::forward<decltype(v)>(v));
		}
	    } else {
		gen_->new_data(std::forward<Data>(data));
	    }
	    return *this;
	}

    private:
	friend class generator;
	generator *gen_;
	explicit gen_output(generator *ctx): gen_(ctx) {}
    };

    class gen_iter_end {};

    class gen_iter {
    public:
	using iterator_category	= std::input_iterator_tag;
	using difference_type	= ptrdiff_t;
	using value_type	= std::remove_reference_t<Output>;
	using pointer		= value_type*;
	using reference		= value_type&;

	gen_iter() = default;
	explicit gen_iter(generator *generator): gen_(generator) {
	    data_ = gen_->next();
	}
	gen_iter(const gen_iter&) = default;
	gen_iter& operator=(const gen_iter&) = default;
	gen_iter(gen_iter&&) = default;
	gen_iter& operator=(gen_iter&&) = default;

	bool operator==(const gen_iter& o) const {
	    return false;
	}

	bool operator!=(const gen_iter& o) const {
	    return !(*this == o);
	}

	bool operator==(const gen_iter_end&) const {
	    return (data_ == nullptr);
	}

	bool operator!=(const gen_iter_end& o) const {
	    return !(*this == o);
	}

	auto& operator++() {
	    if (data_ != nullptr) {
		data_ = gen_->next();
	    }
	    return *this;
	}

	decltype(auto) operator*() {
	    return *data_;
	}

	decltype(auto) operator*() const {
	    return *data_;
	}

    private:
	generator *gen_ = nullptr;
	std::remove_reference_t<Output> *data_ = nullptr;
    };

    explicit generator(size_t buff_size): out_(this) {
	func_.ctx.stack_area = func_.stack.data();
	func_.ctx.stack_size = StackSize;
	buff_.reserve(buff_size);
    }

    generator(size_t buff_size,
	      std::function<void(gen_output&)> func): out_(this) {
	func_.ctx.stack_area = func_.stack.data();
	func_.ctx.stack_size = StackSize;
	buff_.reserve(buff_size);
	start(func);
    }

    generator(const generator&) = delete;
    generator& operator=(const generator&) = delete;
    generator(generator&&) = delete;
    generator& operator=(generator&&) = delete;

    generator& start(std::function<void(gen_output&)> func) {
	c7_getcontext(&func_.ctx);
	c7_makecontext(&func_.ctx,
		       generator::entry_point0,
		       this);
	func_.entry_point = func;
	buff_.clear();
	index_ = 0;
	return *this;
    }

    auto begin() {
	return gen_iter(this);
    }

    auto end() {
	return gen_iter_end{};
    }

private:
    friend class gen_output;
    friend class gen_iter;

    struct {
	std::array<uint64_t, StackSize/sizeof(uint64_t)> stack;
	c7_context_data_t ctx;
	std::function<void(gen_output&)> entry_point;
    } func_;

    struct {
	c7_context_data_t ctx;
    } main_;

    using hold_type = std::conditional_t<std::is_reference_v<Output>,
					 std::remove_reference_t<Output>*,
					 Output>;
    std::vector<hold_type> buff_;
    gen_output out_;
    size_t index_;

    template <typename Data>
    void new_data(Data&& data) {
	if constexpr (std::is_reference_v<Output>) {
	    buff_.push_back(&data);
	} else {
	    buff_.push_back(std::forward<Data>(data));
	}
	if (buff_.size() == buff_.capacity()) {
	    yield_to_main();
	    buff_.clear();
	}
    }

    void yield_to_co() {
	c7_swapcontext(&main_.ctx, &func_.ctx);
    }

    void yield_to_main() {
	c7_swapcontext(&func_.ctx, &main_.ctx);
    }

    std::remove_reference_t<Output> *next() {
	if (index_ == buff_.size()) {
	    yield_to_co();
	    index_ = 0;
	}
	if (buff_.empty()) {
	    return nullptr;
	}
	if constexpr (std::is_reference_v<Output>) {
	    return buff_[index_++];
	} else {
	    return &buff_[index_++];
	}
    }

    void entry_point1() {
	func_.entry_point(out_);
	for (;;) {
	    yield_to_main();
	    buff_.clear();
	}
    }

    static void entry_point0(void *self) {
	reinterpret_cast<generator*>(self)->entry_point1();
    }
};


} // namespace c7::r2


#endif // c7generator_r2.hpp
