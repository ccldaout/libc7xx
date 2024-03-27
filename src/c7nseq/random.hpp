/*
 * c7nseq/random.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * (nothing)
 */
#ifndef C7_NSEQ_RANDOM_HPP_LOADED_
#define C7_NSEQ_RANDOM_HPP_LOADED_


#include <random>
#include <c7nseq/_cmn.hpp>


namespace c7::nseq {


template <typename T>
class random_generator_base {
public:
    virtual T value() = 0;
};


template <typename Engine, typename Dist, typename T>
class random_generator: public random_generator_base<T> {
private:
    Engine engine_;
    Dist dist_;

public:
    template <typename Eng, typename... Args>
    random_generator(Eng engine, Args... args):
	engine_(std::forward<Eng>(engine)),  dist_(std::forward<Args>(args)...) {
    }
    
    T value() override {
	return dist_(engine_);
    }
};



template <typename T, typename D, typename E, typename... Args>
static std::shared_ptr<random_generator_base<T>>
make_random_generator(E engine, Args... args)
{
    using Engine = std::remove_reference_t<E>;
    using Dist   = std::remove_reference_t<D>;
    using Gen    = random_generator<Engine, Dist, T>;
    return std::make_shared<Gen>(std::forward<E>(engine),
				 std::forward<Args>(args)...);
}


class random_iter_end {};


template <typename T>
class random_iter {
private:
    std::shared_ptr<random_generator_base<T>> gen_;
    T val_;

public:
    // for STL iterator
    using iterator_category	= std::input_iterator_tag;
    using difference_type	= ptrdiff_t;
    using value_type		= T;
    using pointer		= value_type*;
    using reference		= value_type&;

    random_iter() = default;
    random_iter(const random_iter&) = default;
    random_iter(random_iter&&) = default;
    random_iter& operator=(const random_iter&) = default;
    random_iter& operator=(random_iter&&) = default;

    explicit random_iter(std::shared_ptr<random_generator_base<T>> gen):
	gen_(std::move(gen)), val_(gen_->value()) {
    }

    bool operator==(const random_iter& o) const {
	return false;
    }

    bool operator!=(const random_iter& o) const {
	return true;
    }

    bool operator==(const random_iter_end& o) const {
	return false;
    }

    bool operator!=(const random_iter_end& o) const {
	return true;
    }

    auto& operator++() {
	val_ = gen_->value();
	return *this;
    }

    auto operator*() {
	return val_;
    }

    auto operator*() const {
	return val_;
    }
};


template <typename T>
class random_seq {
private:
    std::shared_ptr<random_generator_base<T>> gen_;

public:
    explicit random_seq(std::shared_ptr<random_generator_base<T>> gen):
	gen_(std::move(gen)) {
    }

    random_seq(const random_seq&) = default;
    random_seq& operator=(const random_seq&) = default;
    random_seq(random_seq&&) = default;
    random_seq& operator=(random_seq&&) = delete;

    auto begin() {
	return random_iter<T>(gen_);
    }

    auto end() {
	return random_iter_end();
    }

    auto begin() const {
	return random_iter<T>(gen_);
    }

    auto end() const {
	return random_iter_end();
    }
};


template <typename V,
	  template <typename...> class D,
	  typename... Args>
auto random_dist(uint_fast32_t seed, Args... args)
{
    if (seed == 0) {
	std::random_device seed_gen;
	seed = seed_gen();
    }
    std::mt19937 engine(seed);
    auto gen = make_random_generator<V, D<V>>(std::move(engine), std::forward<Args>(args)...);
    return random_seq<V>(std::move(gen));
}


template <typename V,
	  typename D,
	  typename... Args>
auto random_dist(uint_fast32_t seed, Args... args)
{
    if (seed == 0) {
	std::random_device seed_gen;
	seed = seed_gen();
    }
    std::mt19937 engine(seed);
    auto gen = make_random_generator<V, D>(std::move(engine), std::forward<Args>(args)...);
    return random_seq<V>(std::move(gen));
}


template <typename T>
auto random_uniform_dist(uint_fast32_t seed, T a, T b)
{
    if constexpr (std::is_integral_v<T>) {
	return random_dist<T, std::uniform_int_distribution>(seed, a, b);
    } else {
	return random_dist<T, std::uniform_real_distribution>(seed, a, b);
    }
}


inline
auto random_bernoulli_dist(uint_fast32_t seed, double true_rate)
{
    return random_dist<bool, std::bernoulli_distribution>(seed, true_rate);
}


template <typename T>
auto random_binomial_dist(uint_fast32_t seed, T trial_n, double success_rate)
{
    return random_dist<T, std::binomial_distribution>(seed, trial_n, success_rate);
}


template <typename R>
auto random_normal_dist(uint_fast32_t seed, R mean, R stddev)
{
    return random_dist<R, std::normal_distribution>(seed, mean, stddev);
}


template <typename R>
auto random_lognormal_dist(uint_fast32_t seed, R mean, R stddev)
{
    return random_dist<R, std::lognormal_distribution>(seed, mean, stddev);
}


template <typename T>
auto random_poisson_dist(uint_fast32_t seed, double mean)
{
    return random_dist<T, std::poisson_distribution>(seed, mean);
}


} // namespace c7::nseq


#if defined(C7_FORMAT_HELPER_HPP_LOADED_)
namespace c7::format_helper {
template <typename T>
struct format_ident<c7::nseq::random_seq<T>> {
    static constexpr const char *name = "random";
};
} // namespace c7::format_helper
#endif


#endif // c7nseq/random.hpp
