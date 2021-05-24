/*
 * c7typefunc.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_TYPEFUNC_HPP_LOADED__
#define C7_TYPEFUNC_HPP_LOADED__
#include <c7common.hpp>


#include <functional>
#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <type_traits>
#include <sys/time.h>


namespace c7 {
namespace typefunc {


/*----------------------------------------------------------------------------
                             basic type function
----------------------------------------------------------------------------*/

// -- unwrapref : std::reference_wrapper<T> -> T&

template <typename T>
struct unwrapref {
    typedef T type;
};

template <typename T>
struct unwrapref<std::reference_wrapper<T>> {
    typedef T& type;
};


/*----------------------------------------------------------------------------
                            type list manipulation
----------------------------------------------------------------------------*/

// -- cons : T, tuple<T1, T2, ...> -> tuple<T, T1, T2, ...>

template <typename Head, typename... Tail>
struct cons {};

template <typename Head, typename... Tail>
struct cons<Head, std::tuple<Tail...>> {
    typedef typename std::tuple<Head, Tail...> type;
};

template <typename Head, typename... Tail>
using cons_t = typename cons<Head, Tail...>::type;


// -- car :       T1, T2, ...  -> T1
// -- car : tuple<T1, T2, ...> -> T1

template <typename Head, typename... Tail>
struct car {
    typedef Head type;
};

template <typename Head, typename... Tail>
struct car<std::tuple<Head, Tail...>> {
    typedef Head type;
};

template <typename Head, typename... Tail>
using car_t = typename car<Head, Tail...>::type;


// -- tail :       T1, T2, ...  -> tuple<T2, ...>
// -- tail : tuple<T1, T2, ...> -> tuple<T2, ...>

template <typename Head, typename... Tail>
struct cdr {
    typedef std::tuple<Tail...> type;
};

template <typename Head, typename... Tail>
struct cdr<std::tuple<Head, Tail...>> {
    typedef std::tuple<Tail...> type;
};

template <typename Head, typename... Tail>
using cdr_t = typename cdr<Head, Tail...>::type;


// -- map : typefunc,       t1, t2, ...  -> tuple<typefunc<t1>, typefunc<t2>, ...>
// -- map : typefunc, tuple<t1, t2, ...> -> tuple<typefunc<t1>, typefunc<t2>, ...>

template <template <typename> class TFunc,
	  typename... Types>
struct map {
    typedef std::tuple<typename TFunc<Types>::type...> type;
};

template <template <typename> class TFunc,
	  typename... Types>
struct map<TFunc, std::tuple<Types...>> {
    typedef typename map<TFunc, Types...>::type type;
};

template <template <typename> class TFunc,
	  typename... Types>
using map_t = typename map<TFunc, Types...>::type;


// -- apply : type, tuple<T1, T2, ...> -> type<T1, T2, ...>

template <template <typename...> class T, typename... Ts>
struct apply {};

template <template <typename...> class T, typename... Ts>
struct apply<T, std::tuple<Ts...>> {
    typedef T<Ts...> type;
};

template <template <typename...> class T, typename... Ts>
using apply_t = typename apply<T, Ts...>::type;


// -- is_empty : 
template <typename... Args>
struct is_empty {
    static constexpr bool value = true;
};

template <typename Head, typename... Tail>
struct is_empty<Head, Tail...> {
    static constexpr bool value = false;
};

template <typename... Args>
inline constexpr bool is_empty_v = is_empty<Args...>::value;


/*----------------------------------------------------------------------------
                            multi-case conditional
----------------------------------------------------------------------------*/

template <typename... Args>
struct ifelse {
    typedef void type;
};

template <typename C, typename T, typename... Args>
struct ifelse<C, T, Args...> {
    typedef typename std::conditional_t<bool(C::value), T, typename ifelse<Args...>::type> type;
};

template <typename T>
struct ifelse<T> {
    typedef T type;
};

template <typename... Args>
using ifelse_t = typename ifelse<Args...>::type;


/*----------------------------------------------------------------------------
                                    others
----------------------------------------------------------------------------*/

template <typename T>
inline constexpr bool false_v = false;


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace typefunc
} // namespace c7


#endif // c7typefunc.hpp
