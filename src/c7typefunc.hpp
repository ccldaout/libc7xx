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

// -- is_empty<...> :

template <typename... Args>
struct is_empty {
    static constexpr bool value = false;
};

template <>
struct is_empty<> {
    static constexpr bool value = true;
};

template <typename... Args>
inline constexpr bool is_empty_v = is_empty<Args...>::value;


// --  count<T, ...> -> number of T,...

template <typename T, typename... Types>
static inline constexpr size_t count_()
{
    if constexpr (is_empty_v<Types...>) {
	return 0;
    } else {
	return 1 + count_<Types...>();
    }
}

template <typename... Types>
static inline constexpr size_t count()
{
    return count_<void, Types...>();
}


// --  at<N, T, ...> -> N th type of <T, ...>

template <int N, typename... Types>
struct at {};

template <typename T, typename... Types>
struct at<0, T, Types...> {
    typedef T type;
};

template <int N, typename T, typename... Types>
struct at<N, T, Types...> {
    typedef typename at<N-1, Types...>::type type;
};

template <int N, typename... Types>
using at_t = typename at<N, Types...>::type;


// -- cons<T, tuple<T2, ...>> -> tuple<T, T2, ...>

template <typename Head, typename... Tail>
struct cons {};

template <typename Head, typename... Tail>
struct cons<Head, std::tuple<Tail...>> {
    typedef typename std::tuple<Head, Tail...> type;
};

template <typename Head, typename... Tail>
using cons_t = typename cons<Head, Tail...>::type;


// -- car<      T1, T2, ...>  -> T1
// -- car<tuple<T1, T2, ...>> -> T1

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


// -- tail<      T1, T2, ...>  -> tuple<T2, ...>
// -- tail<tuple<T1, T2, ...>> -> tuple<T2, ...>

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


// -- map<TypeFunc,       T, ...>  -> tuple<typefunc<T>, ...>
// -- map<TypeFunc, tuple<T, ...>> -> tuple<typefunc<T>, ...>

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


// -- apply<Type, tuple<T, ...>> -> Type<T, ...>

template <template <typename...> class T, typename... Ts>
struct apply {};

template <template <typename...> class T, typename... Ts>
struct apply<T, std::tuple<Ts...>> {
    typedef T<Ts...> type;
};

template <template <typename...> class T, typename... Ts>
using apply_t = typename apply<T, Ts...>::type;


/*----------------------------------------------------------------------------
                            multi-case conditional
----------------------------------------------------------------------------*/

// -- ifelse<CondType1, SelectedType1,
//           ...
//           CondTypeN, SelectedTypeN> -> one of SelectedType1, ..., SelectedTypeN, void
//
// -- ifelse<CondType1, SelectedType1,
//           ...
//           CondTypeN, SelectedTypeN,
//           DefaultType>              -> one of SelectedType1, ..., SelectedTypeN, DefaultType
//
// -- ifelse<CondType1, SelectedType1,
//           ...
//           CondTypeN, SelectedTypeN,
//           std::true_type, DefaultType> -> one of SelectedType1, ..., SelectedTypeN, DefaultType

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


// remove cv and reference: available for inner type of tuple, pair
template <typename T>
struct remove_cref {
    using type = std::remove_const_t<std::remove_reference_t<T>>;
};

template <typename T>
using remove_cref_t = typename remove_cref<T>::type;

template <typename First, typename Second>
struct remove_cref<std::pair<First, Second>> {
    using type = std::pair<std::remove_const_t<std::remove_reference_t<First>>,
			   std::remove_const_t<std::remove_reference_t<Second>>>;
};

template <typename... Types>
struct remove_cref<std::tuple<Types...>> {
    using type = std::tuple<remove_cref_t<Types>...>;
};


// first/second type of std::pair
template <typename T>
using first_type_t = typename T::first_type;
template <typename T>
using second_type_t = typename T::second_type;


// begin/end iterator type
template <typename T>
using begin_iter_type = decltype(std::declval<T>().begin());
template <typename T>
using end_iter_type = decltype(std::declval<T>().end());


// count for tuple
template <typename>
struct count_of_tuple {};
template <typename... Types>
struct count_of_tuple<std::tuple<Types...>> {
    static constexpr int value = count<Types...>();
};
template <typename T>
inline constexpr int count_of_tuple_v =
    count_of_tuple<std::remove_const_t<std::remove_reference_t<T>>>::value;


// is_iterable
struct is_iterable_impl {
    template <typename T>
    static auto check(T* o) -> decltype(
	std::begin(*o),
	std::true_type{});

    template <typename T>
    static auto check(...) -> std::false_type;
};
template <typename T>
struct is_iterable:
    decltype(is_iterable_impl::check<c7::typefunc::remove_cref_t<T>>(nullptr)) {};
template <typename T>
inline constexpr bool is_iterable_v = is_iterable<T>::value;


// is_sequence_of
template <typename Seq, typename T>
inline constexpr bool is_sequence_of_impl(
    std::enable_if_t<c7::typefunc::is_iterable_v<Seq>>*)
{
    return std::is_same_v<
	c7::typefunc::remove_cref_t<decltype(*std::begin(std::declval<Seq>()))>,
	c7::typefunc::remove_cref_t<T>>;
}

template <typename Seq, typename T>
inline constexpr bool is_sequence_of_impl(...)
{
    return false;
}

template <typename Seq, typename T>
struct is_sequence_of {
    static constexpr bool value = is_sequence_of_impl<Seq, T>(nullptr);
};

template <typename Seq, typename T>
inline constexpr bool is_sequence_of_v = is_sequence_of<Seq, T>::value;


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace typefunc
} // namespace c7


#endif // c7typefunc.hpp
