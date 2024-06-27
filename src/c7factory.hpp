/*
 * c7factory.hpp
 *
 * Copyright (c) 2024 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_FACTORY_HPP_LOADED_
#define C7_FACTORY_HPP_LOADED_
#include <c7common.hpp>


namespace c7::factory_type1 {


// utility: type function
template <typename Target>
struct target_config_type {
    using type = typename Target::config_type;
};
template <typename Target>
using target_config_type_t = typename target_config_type<Target>::type;


// template to build abstract base class of factroy

template <typename TargetInterface, typename... Configs>
struct interface;

template <typename TargetInterface>
struct interface<TargetInterface> {
    virtual ~interface() {}
    virtual void make() const = 0;
};

template <typename TargetInterface, typename Config, typename... Configs>
struct interface<TargetInterface, Config, Configs...>:
	public interface<TargetInterface, Configs...> {

    using interface<TargetInterface, Configs...>::make;

    virtual std::unique_ptr<TargetInterface> make(const Config& c) const = 0;
};


// template to build factroy
//
// [Requirement] Target::config_type is type of argument of constructor

template <typename TargetInterface, typename Base, typename... Target>
struct factory_impl;

template <typename TargetInterface, typename Base>
struct factory_impl<TargetInterface, Base>: public Base {
    using Base::make;
    void make() const override {}
};

template <typename TargetInterface, typename Base, typename Target, typename... Targets>
struct factory_impl<TargetInterface, Base, Target, Targets...>:
	public factory_impl<TargetInterface, Base, Targets...> {

    using factory_impl<TargetInterface, Base, Targets...>::make;

    std::unique_ptr<TargetInterface> make(const target_config_type_t<Target>& c) const override {
	return std::make_unique<Target>(c);
    }
};

template <typename TargetInterface, typename... Targets>
struct factory:
	public factory_impl<TargetInterface,
			    interface<TargetInterface,
				      target_config_type_t<Targets>...>,
			    Targets...> {
    using factory_impl<TargetInterface,
		       interface<TargetInterface,
				 target_config_type_t<Targets>...>,
		       Targets...>::make;
};


} // namespace c7::factory_type1


namespace c7::factory_type1c {


template <typename TargetInterface, typename Base, typename... Configs>
struct factory_impl;

template <typename TargetInterface, typename Base>
struct factory_impl<TargetInterface, Base>: public Base {
    using Base::make;
    void set_maker() {}
    void assign() {}
    void make() const override {}
};

template <typename TargetInterface, typename Base, typename Config, typename... Configs>
struct factory_impl<TargetInterface, Base, Config, Configs...>:
	public factory_impl<TargetInterface, Base, Configs...> {
private:
    using ret_type = std::unique_ptr<TargetInterface>;
    std::function<ret_type(const Config&)> maker_;

public:
    using factory_impl<TargetInterface, Base, Configs...>::set_maker;
    using factory_impl<TargetInterface, Base, Configs...>::assign;
    using factory_impl<TargetInterface, Base, Configs...>::make;

    template <typename Target>
    const auto& set_maker(const Config&) {
	maker_ = [](const Config& c){ return std::make_unique<Target>(c); };
	return maker_;
    }

    void assign(std::function<ret_type(const Config&)> maker) {
	maker_ = maker;
    }

    ret_type make(const Config& c) const override {
	return maker_(c);
    }
};

template <typename TargetInterface, typename... Configs>
struct factory:
	public factory_impl<TargetInterface,
			    factory_type1::interface<TargetInterface, Configs...>,
			    Configs...> {
private:
    using base_type = factory_impl<TargetInterface,
				   factory_type1::interface<TargetInterface, Configs...>,
				   Configs...>;
    using base_type::set_maker;

public:
    using base_type::assign;
    using base_type::make;

    template <typename Target>
    const auto& set() {
	return this->template set_maker<Target>(typename Target::config_type{});
    }
};


} // namespace c7::factory_type1c;


namespace c7::factory_type2 {


template <typename T>
struct tag {};


template <typename... Itfs>
struct interface_impl;

template <>
struct interface_impl<> {
    virtual ~interface_impl() {}
    virtual void make_() const = 0;
};

template <typename Itf, typename... Itfs>
struct interface_impl<Itf, Itfs...>:
	public interface_impl<Itfs...> {
    using interface_impl<Itfs...>::make_;
    virtual std::unique_ptr<Itf> make_(tag<Itf>) const = 0;
};

template <typename... Itfs>
struct interface:
	public interface_impl<Itfs...> {
    template <typename Itf>
    std::unique_ptr<Itf> make() const {
	return this->make_(tag<Itf>{});
    }

};


template <int N, typename...>
struct at;

template <int N, typename... Itfs>
struct at<N, interface<Itfs...>> {
    using type = typename c7::typefunc::at_t<N, Itfs...>;
};

template <int N, typename T>
using at_t = typename at<N, T>::type;


template <int N, typename FactoryItfs, typename... Items>
struct factory_impl;

template <int N, typename FactoryItfs>
struct factory_impl<N, FactoryItfs>:
	public FactoryItfs {
    void make_() const {}
};

template <int N, typename FactoryItfs, typename Item, typename... Items>
struct factory_impl<N, FactoryItfs, Item, Items...>:
	public factory_impl<N+1, FactoryItfs, Items...> {
private:
    using Itf = at_t<N, FactoryItfs>;
public:
    using factory_impl<N+1, FactoryItfs, Items...>::make_;
    std::unique_ptr<Itf> make_(tag<Itf>) const {
	return std::make_unique<Item>();
    }
};

template <typename FactoryItfs, typename... Items>
struct factory:
	public factory_impl<0, FactoryItfs, Items...> {
};


} // namespace c7::factory_type2;


namespace c7::factory_type3 {


template <typename... Itfs>
struct interface;

template <>
struct interface<> {
    virtual ~interface() {}
    virtual void make() const = 0;
};

template <typename Itf, typename... Itfs>
struct interface<Itf, Itfs...>:
	public interface<Itfs...> {
    using interface<Itfs...>::make;
    virtual std::unique_ptr<Itf> make(const typename Itf::config_type&) const = 0;
};


template <int N, typename...>
struct at;

template <int N, typename... Itfs>
struct at<N, interface<Itfs...>> {
    using type = typename c7::typefunc::at_t<N, Itfs...>;
};

template <int N, typename T>
using at_t = typename at<N, T>::type;


template <int N, typename FactoryItfs, typename... Items>
struct factory_impl;

template <int N, typename FactoryItfs>
struct factory_impl<N, FactoryItfs>:
	public FactoryItfs {
    void make() const {}
};

template <int N, typename FactoryItfs, typename Item, typename... Items>
struct factory_impl<N, FactoryItfs, Item, Items...>:
	public factory_impl<N+1, FactoryItfs, Items...> {
private:
    using Itf = at_t<N, FactoryItfs>;
public:
    using factory_impl<N+1, FactoryItfs, Items...>::make;
    std::unique_ptr<Itf> make(const typename Itf::config_type& c) const {
	return std::make_unique<Item>(c);
    }
};

template <typename FactoryItfs, typename... Items>
struct factory:
	public factory_impl<0, FactoryItfs, Items...> {
};


} // namespace c7::factory_type3;


#endif // c7factory.hpp
