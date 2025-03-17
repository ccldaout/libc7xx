/*
 * c7factory.hpp
 *
 * Copyright (c) 2024 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google document:
 * https://docs.google.com/document/d/1D6zyVF108nXGdnHovTmbNC4-0M9YdNFaYdAIS8ayPic/edit?usp=sharing
 */
#ifndef C7_FACTORY_HPP_LOADED_
#define C7_FACTORY_HPP_LOADED_
#include <c7common.hpp>


#include <memory>


namespace c7::factory_util {


// utility: smart pointer traits

template <template <typename ...> class SmartPtr>
struct smartptr_traits;

template <>
struct smartptr_traits<std::shared_ptr> {
    template <typename T>
    using type = std::shared_ptr<T>;

    template <typename T, typename... Args>
    static type<T> make(Args&&... args) {
	return std::make_shared<T>(std::forward<Args>(args)...);
    }
};

template <>
struct smartptr_traits<std::unique_ptr> {
    template <typename T>
    using type = std::unique_ptr<T>;

    template <typename T, typename... Args>
    static type<T> make(Args&&... args) {
	return std::make_unique<T>(std::forward<Args>(args)...);
    }
};


// utility: type function: Target -> configure type of Target

template <typename Target>
struct target_config {
    using type = typename Target::config_type;
};

template <typename Target>
using target_config_t = typename target_config<Target>::type;


} // c7::factory_util


//------------------------------------------------------------------------------


namespace c7::factory_type1 {


using namespace c7::factory_util;


// template to build abstract base class of factroy

template <template <typename...> class SmartPtr,
	  typename...>
struct interface_impl;

template <template <typename...> class SmartPtr,
	  typename TargetInterface>
struct interface_impl<SmartPtr, TargetInterface> {
    virtual ~interface_impl() {}
    virtual void make() const = 0;
};

template <template <typename...> class SmartPtr,
	  typename TargetInterface,
	  typename Config, typename... Configs>
struct interface_impl<SmartPtr, TargetInterface, Config, Configs...>:
	public interface_impl<SmartPtr, TargetInterface, Configs...> {

    using interface_impl<SmartPtr, TargetInterface, Configs...>::make;
    using traits = smartptr_traits<SmartPtr>;

    virtual typename traits::type<TargetInterface> make(const Config& c) const = 0;
};

template <template <typename...> class SmartPtr,
	  typename... T>
using interface_x = interface_impl<SmartPtr, T...>;

template <typename... T>
using interface = interface_x<std::unique_ptr, T...>;

template <typename... T>
using interface_sp = interface_x<std::shared_ptr, T...>;


// template to build factroy
//
// [Requirement] Target::config_type is type of argument of constructor

template <template <typename...> class SmartPtr,
	  typename...>
struct factory_impl;

template <template <typename...> class SmartPtr,
	  typename TargetInterface,
	  typename FactoryInterface>
struct factory_impl<SmartPtr, TargetInterface, FactoryInterface>: public FactoryInterface {
    using FactoryInterface::make;
    void make() const override {}
};

template <template <typename...> class SmartPtr,
	  typename TargetInterface,
	  typename FactoryInterface,
	  typename Target, typename... Targets>
struct factory_impl<SmartPtr, TargetInterface, FactoryInterface, Target, Targets...>:
	public factory_impl<SmartPtr, TargetInterface, FactoryInterface, Targets...> {

    using factory_impl<SmartPtr, TargetInterface, FactoryInterface, Targets...>::make;
    using traits = smartptr_traits<SmartPtr>;

    typename traits::type<TargetInterface> make(const target_config_t<Target>& c) const override {
	return traits::template make<Target>(c);
    }
};

template <template <typename...> class SmartPtr,
	  typename TargetInterface,
	  typename... Targets>
using factory_x = factory_impl<SmartPtr,
			       TargetInterface,
			       interface_x<SmartPtr, TargetInterface, target_config_t<Targets>...>,
			       Targets...>;

template <typename TargetInterface,
	  typename... Targets>
using factory = factory_x<std::unique_ptr, TargetInterface, Targets...>;

template <typename TargetInterface,
	  typename... Targets>
using factory_sp = factory_x<std::shared_ptr, TargetInterface, Targets...>;


} // namespace c7::factory_type1


//------------------------------------------------------------------------------


namespace c7::factory_type1c {


using namespace c7::factory_util;


template <template <typename...> class SmartPtr,
	  typename TargetInterface,
	  typename FactoryInterface,
	  typename... Configs>
struct factory_impl;

template <template <typename...> class SmartPtr,
	  typename TargetInterface,
	  typename FactoryInterface>
struct factory_impl<SmartPtr, TargetInterface, FactoryInterface>:
	public FactoryInterface {
    using FactoryInterface::make;
    void set_maker() {}
    void assign() {}
    void make() const override {}
};

template <template <typename...> class SmartPtr,
	  typename TargetInterface,
	  typename FactoryInterface,
	  typename Config, typename... Configs>
struct factory_impl<SmartPtr, TargetInterface, FactoryInterface, Config, Configs...>:
	public factory_impl<SmartPtr, TargetInterface, FactoryInterface, Configs...> {
private:
    using ret_type = typename smartptr_traits<SmartPtr>::type<TargetInterface>;
    std::function<ret_type(const Config&)> maker_;

public:
    using factory_impl<SmartPtr, TargetInterface, FactoryInterface, Configs...>::set_maker;
    using factory_impl<SmartPtr, TargetInterface, FactoryInterface, Configs...>::assign;
    using factory_impl<SmartPtr, TargetInterface, FactoryInterface, Configs...>::make;

    template <typename Target>
    const auto& set_maker(const Config&) {
	maker_ = [](const Config& c){
		     return smartptr_traits<SmartPtr>::template make<Target>(c);
		 };
	return maker_;
    }

    void assign(std::function<ret_type(const Config&)> maker) {
	maker_ = maker;
    }

    ret_type make(const Config& c) const override {
	return maker_(c);
    }
};

template <template <typename...> class SmartPtr,
	  typename TargetInterface,
	  typename... Configs>
struct factory_x: public factory_impl<SmartPtr,
				      TargetInterface,
				      factory_type1::interface_x<SmartPtr, TargetInterface, Configs...>,
				      Configs...> {
    template <typename Target>
    const auto& set() {
	return this->template set_maker<Target>(typename Target::config_type{});
    }
};

template <typename TargetInterface,
	  typename... Configs>
using factory = factory_x<std::unique_ptr, TargetInterface, Configs...>;

template <typename TargetInterface,
	  typename... Configs>
using factory_sp = factory_x<std::shared_ptr, TargetInterface, Configs...>;


} // namespace c7::factory_type1c;


//------------------------------------------------------------------------------


namespace c7::factory_type2 {


using namespace c7::factory_util;


template <typename T>
struct tag {};


// interface (type2)

template <template <typename...> class SmartPtr,
	  typename... Itfs>
struct interface_impl;

template <template <typename...> class SmartPtr>
struct interface_impl<SmartPtr> {
    virtual ~interface_impl() {}
    virtual void make_() const = 0;
};

template <template <typename...> class SmartPtr,
	  typename Itf, typename... Itfs>
struct interface_impl<SmartPtr, Itf, Itfs...>:
	public interface_impl<SmartPtr, Itfs...> {
    using interface_impl<SmartPtr, Itfs...>::make_;
    using pointer_type = typename smartptr_traits<SmartPtr>::type<Itf>;
    virtual pointer_type make_(tag<Itf>) const = 0;
};

template <template <typename...> class SmartPtr,
	  typename... Itfs>
struct interface_x:
	public interface_impl<SmartPtr, Itfs...> {
    template <typename Itf>
    auto make() const {
	return this->make_(tag<Itf>{});
    }

};

template <typename... Itfs>
using interface = interface_x<std::unique_ptr, Itfs...>;

template <typename... Itfs>
using interface_sp = interface_x<std::shared_ptr, Itfs...>;


// at (type2)

template <int N,
	  typename...>
struct at;

template <int N,
	  template <typename...> class SmartPtr,
	  typename... Itfs>
struct at<N, interface_x<SmartPtr, Itfs...>> {
    using type = typename c7::typefunc::at_t<N, Itfs...>;
};

template <int N, typename T>
using at_t = typename at<N, T>::type;


// factory_impl (type2)

template <template <typename...> class SmartPtr,
	  int N,
	  typename FactoryItfs,
	  typename... Items>
struct factory_impl;

template <template <typename...> class SmartPtr,
	  int N,
	  typename FactoryItfs>
struct factory_impl<SmartPtr, N, FactoryItfs>:
	public FactoryItfs {
    void make_() const {}
};

template <template <typename...> class SmartPtr,
	  int N,
	  typename FactoryItfs,
	  typename Item, typename... Items>
struct factory_impl<SmartPtr, N, FactoryItfs, Item, Items...>:
	public factory_impl<SmartPtr, N+1, FactoryItfs, Items...> {
private:
    using Itf = at_t<N, FactoryItfs>;
public:
    using factory_impl<SmartPtr, N+1, FactoryItfs, Items...>::make_;
    using pointer_type = typename smartptr_traits<SmartPtr>::type<Itf>;
    pointer_type make_(tag<Itf>) const {
	return smartptr_traits<SmartPtr>::template make<Item>();
    }
};

template <template <typename...> class SmartPtr,
	  typename FactoryItfs,
	  typename... Items>
using factory_x = factory_impl<SmartPtr, 0, FactoryItfs, Items...>;

template <typename FactoryItfs,
	  typename... Items>
using factory = factory_x<std::unique_ptr, FactoryItfs, Items...>;

template <typename FactoryItfs,
	  typename... Items>
using factory_sp = factory_x<std::shared_ptr, FactoryItfs, Items...>;


} // namespace c7::factory_type2;


//------------------------------------------------------------------------------


namespace c7::factory_type3 {


using namespace c7::factory_util;


// interface (type3)

template <template <typename...> class SmartPtr,
	  typename... Itfs>
struct interface_impl;

template <template <typename...> class SmartPtr>
struct interface_impl<SmartPtr> {
    virtual ~interface_impl() {}
    virtual void make() const = 0;
};

template <template <typename...> class SmartPtr,
	  typename Itf, typename... Itfs>
struct interface_impl<SmartPtr, Itf, Itfs...>:
	public interface_impl<SmartPtr, Itfs...> {
    using interface_impl<SmartPtr, Itfs...>::make;
    using pointer_type = typename smartptr_traits<SmartPtr>::type<Itf>;
    virtual pointer_type make(const typename Itf::config_type&) const = 0;
};

template <template <typename...> class SmartPtr,
	  typename... Itfs>
using interface_x = interface_impl<SmartPtr, Itfs...>;

template <typename... Itfs>
using interface = interface_x<std::unique_ptr, Itfs...>;

template <typename... Itfs>
using interface_sp = interface_x<std::shared_ptr, Itfs...>;


// at (type3)

template <int N,
	  typename...>
struct at;

template <int N,
	  template <typename...> class SmartPtr,
	  typename... Itfs>
struct at<N, interface_x<SmartPtr, Itfs...>> {
    using type = typename c7::typefunc::at_t<N, Itfs...>;
};

template <int N, typename T>
using at_t = typename at<N, T>::type;


// factory_impl (type3)

template <template <typename...> class SmartPtr,
	  int N, typename FactoryItfs, typename... Items>
struct factory_impl;

template <template <typename...> class SmartPtr,
	  int N,
	  typename FactoryItfs>
struct factory_impl<SmartPtr, N, FactoryItfs>:
	public FactoryItfs {
    void make() const {}
};

template <template <typename...> class SmartPtr,
	  int N,
	  typename FactoryItfs,
	  typename Item, typename... Items>
struct factory_impl<SmartPtr, N, FactoryItfs, Item, Items...>:
	public factory_impl<SmartPtr, N+1, FactoryItfs, Items...> {
private:
    using Itf = at_t<N, FactoryItfs>;
public:
    using factory_impl<SmartPtr, N+1, FactoryItfs, Items...>::make;
    using pointer_type = typename smartptr_traits<SmartPtr>::type<Itf>;
    pointer_type make(const typename Itf::config_type& c) const {
	return smartptr_traits<SmartPtr>::template make<Item>(c);
    }
};

template <template <typename...> class SmartPtr,
	  typename FactoryItfs,
	  typename... Items>
using factory_x = factory_impl<SmartPtr, 0, FactoryItfs, Items...>;

template <typename FactoryItfs,
	  typename... Items>
using factory = factory_x<std::unique_ptr, FactoryItfs, Items...>;

template <typename FactoryItfs,
	  typename... Items>
using factory_sp = factory_x<std::shared_ptr, FactoryItfs, Items...>;


} // namespace c7::factory_type3;


//------------------------------------------------------------------------------


#endif // c7factory.hpp
