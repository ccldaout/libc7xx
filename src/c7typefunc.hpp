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
#ifndef C7_TYPEFUNC_HPP_LOADED_
#define C7_TYPEFUNC_HPP_LOADED_
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
                            type list manipulation
----------------------------------------------------------------------------*/

// type_list

template <typename... Ts>
struct type_list {};


// -- is_empty_v<          Ts... >  -> bool
// -- is_empty_v<type_list<Ts...>>  -> bool

template <typename... Ts>
struct is_empty: public std::false_type {};

template <>
struct is_empty<>: public std::true_type {};

template <>
struct is_empty<type_list<>>: public std::true_type {};

template <typename... Ts>
inline constexpr bool is_empty_v = is_empty<Ts...>::value;


// --  count<T, ...>() -> number of T,...

template <typename... Ts>
static inline constexpr size_t count()
{
    return sizeof...(Ts);
}


// - count_v<          Ts... > -> number of Ts...
// - count_v<type_list<Ts...>> -> number of Ts...

template <typename... Ts>
struct countof {
    static constexpr size_t value = sizeof...(Ts);
};

template <typename... Ts>
struct countof<type_list<Ts...>> {
    static constexpr size_t value = sizeof...(Ts);
};

template <typename... Ts>
inline constexpr size_t count_v = countof<Ts...>::value;


// -- count_of_tuple_v<tuple<Ts...>> -> count of Ts...

template <typename>
struct count_of_tuple {};

template <typename... Ts>
struct count_of_tuple<std::tuple<Ts...>> {
    static constexpr int value = sizeof...(Ts);
};

template <typename T>
inline constexpr int count_of_tuple_v =
    count_of_tuple<std::remove_const_t<std::remove_reference_t<T>>>::value;


// -- match_oneof_v<P, T,           Ts... > -> bool
// -- match_oneof_v<P, T, type_list<Ts...>> -> bool

template <template <typename, typename> class, typename... Ts>
struct match_oneof;

template <template <typename, typename> class P, typename T>
struct match_oneof<P, T>: public std::false_type {};

template <template <typename, typename> class P, typename T, typename U, typename... Ts>
struct match_oneof<P, T, U, Ts...> {
    static constexpr int value = (P<T, U>::value || match_oneof<P, T, Ts...>::value);
};

template <template <typename, typename> class P, typename T, typename... Ts>
struct match_oneof<P, T, type_list<Ts...>>:
	public match_oneof<P, T, Ts...> {};

template <template <typename, typename> class P, typename... Ts>
inline constexpr bool match_oneof_v = match_oneof<P, Ts...>::value;


// -- is_oneof_v<T,           Ts... > -> bool
// -- is_oneof_v<T, type_list<Ts...>> -> bool

template <typename... Ts>
inline constexpr bool is_oneof_v = match_oneof_v<std::is_same, Ts...>;


// --  at<N,           Ts... > -> type of N th Ts...
// --  at<N, type_list<Ts...>> -> type of N th Ts...

template <int N, typename... Ts>
struct at {};

template <typename T, typename... Ts>
struct at<0, T, Ts...> {
    using type = T;
};

template <int N, typename T, typename... Ts>
struct at<N, T, Ts...>:
	public at<N-1, Ts...> {};

template <int N, typename... Ts>
struct at<N, type_list<Ts...>>:
	public at<N, Ts...> {};

template <int N, typename... Ts>
using at_t = typename at<N, Ts...>::type;


// -- cons<T,           Ts... > -> type_list<T, Ts...>
// -- cons<T, type_list<Ts...>> -> type_list<T, Ts...>
// -- cons<T, tuple    <Ts...>> -> tuple    <T, Ts...>

template <typename T, typename... Ts>
struct cons: public cons<T, type_list<Ts...>> {};

template <typename T, typename... Ts>
struct cons<T, type_list<Ts...>> {
    using type = type_list<T, Ts...>;
};

template <typename T, typename... Ts>
struct cons<T, std::tuple<Ts...>> {
    using type = typename std::tuple<T, Ts...>;
};

template <typename T, typename... Ts>
using cons_t = typename cons<T, Ts...>::type;


// -- car<          T, Ts...>  -> T
// -- car<type_list<T, Ts...>> -> T
// -- car<tuple    <T, Ts...>> -> T

template <typename T, typename... Ts>
struct car {
    using type = T;
};

template <typename T, typename... Ts>
struct car<type_list<T, Ts...>> {
    using type = T;
};

template <typename T, typename... Ts>
struct car<std::tuple<T, Ts...>> {
    using type = T;
};

template <typename T, typename... Ts>
using car_t = typename car<T, Ts...>::type;


// -- cdr<          T, Ts... > -> type_list<Ts...>
// -- cdr<type_list<T, Ts...>> -> type_list<Ts...>
// -- cdr<tuple    <T, Ts...>> -> tuple    <Ts...>

template <typename... Ts>
struct cdr: public cdr<type_list<Ts...>> {};

template <typename T, typename... Ts>
struct cdr<type_list<T, Ts...>> {
    using type = type_list<Ts...>;
};

template <typename T, typename... Ts>
struct cdr<std::tuple<T, Ts...>> {
    using type = std::tuple<Ts...>;
};

template <typename T, typename... Ts>
using cdr_t = typename cdr<T, Ts...>::type;


// -- map<F,           Ts... > -> type_list<F<Ts>...>
// -- map<F, type_list<Ts...>> -> type_list<F<Ts>...>
// -- map<F, tuple    <Ts...>> -> tuple    <F<Ts>...>

template <template <typename> class F, typename... Ts>
struct map: public map<F, type_list<Ts...>> {};

template <template <typename> class F, typename... Ts>
struct map<F, type_list<Ts...>> {
    using type = type_list<F<Ts>...>;
};

template <template <typename> class F, typename... Ts>
struct map<F, std::tuple<Ts...>> {
    using type = std::tuple<F<Ts>...>;
};

template <template <typename> class F, typename... Ts>
using map_t = typename map<F, Ts...>::type;


// -- apply<F,           Ts... > -> F<Ts...>
// -- apply<F, type_list<Ts...>> -> F<Ts...>
// -- apply<F, tuple    <Ts...>> -> F<Ts...>

template <template <typename...> class F, typename... Ts>
struct apply {
    using type = F<Ts...>;
};

template <template <typename...> class F, typename... Ts>
struct apply<F, type_list<Ts...>>: public apply<F, Ts...> {};

template <template <typename...> class F, typename... Ts>
struct apply<F, std::tuple<Ts...>>: public apply<F, Ts...> {};

template <template <typename...> class F, typename... Ts>
using apply_t = typename apply<F, Ts...>::type;


// -- extract_type_list<F<Ts...>> -> type_list<Ts...>

template <typename>
struct extract_type_list;

template <template <typename...> class F, typename... Ts>
struct extract_type_list<F<Ts...>> {
    using type = type_list<Ts...>;
};

template <typename T>
using extract_type_list_t = typename extract_type_list<T>::type;


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

template <typename... Ts>
struct ifelse {
    using type = void;
};

template <typename C, typename T, typename... Ts>
struct ifelse<C, T, Ts...> {
    using type = typename std::conditional_t<bool(C::value), T, typename ifelse<Ts...>::type>;
};

template <typename T>
struct ifelse<T> {
    using type = T;
};

template <typename... Ts>
using ifelse_t = typename ifelse<Ts...>::type;


/*----------------------------------------------------------------------------
                                    others
----------------------------------------------------------------------------*/

// -- false_v<T> -> false

template <typename T>
inline constexpr bool false_v = false;


// -- is_derived_from_v<D, B> -> bool (D is derived from B)

template <typename D, typename B>
struct is_derived_from {
    static constexpr bool value = std::is_base_of_v<B, D>;
};

template <typename D, typename B>
inline constexpr bool is_derived_from_v = is_derived_from<D, B>::value;


// -- unwrapref -> std::reference_wrapper<T> -> T&

template <typename T>
struct unwrapref {
    using type = T;
};

template <typename T>
struct unwrapref<std::reference_wrapper<T>> {
    using type = T&;
};


// -- remove_cref_t<T>                -> T'
// -- remove_cref_t<pair<T, U>>       -> pair<T', U'>
// -- remove_cref_t<type_list<Ts...>> -> type_list<Ts'...>
// -- remove_cref_t<tuple    <Ts...>> -> tuple    <Ts'...>
//    T', U', Ts'... have no cv- and no refrence.

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

template <typename... Ts>
struct remove_cref<type_list<Ts...>> {
    using type = type_list<remove_cref_t<Ts>...>;
};

template <typename... Ts>
struct remove_cref<std::tuple<Ts...>> {
    using type = std::tuple<remove_cref_t<Ts>...>;
};


// --  first_type_t<pair<T,U>> -> T
// -- second_type_t<pair<T,U>> -> U

template <typename T>
using first_type_t = typename T::first_type;
template <typename T>
using second_type_t = typename T::second_type;


// -- begin_iter_type<T> -> type of begin iterator of T
// --   end_iter_type<T> -> type of  end  iterator of T

template <typename T>
using begin_iter_type = decltype(std::declval<T>().begin());
template <typename T>
using end_iter_type = decltype(std::declval<T>().end());


// -- is_iterable_v<T> -> bool

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


// -- is_sequence_of_v<S, T> -> bool (type of iterated value from S is T)

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


// -- is_buffer_type_v<T> -> bool

struct is_buffer_type_impl {
    template <typename T>
    static auto check(const T *o) -> decltype(
	(*o)[0],
	o->data(),
	o->size(),
	std::true_type{});

    template <typename T>
    static auto check(...) -> std::false_type;
};

template <typename T>
struct is_buffer_type:
	decltype(is_buffer_type_impl::check<T>(nullptr)) {};

template <typename T>
inline constexpr bool is_buffer_type_v = is_buffer_type<T>::value;


// -- macro generate following type function:
// -- has_MEMBERNAME_v<T> -> bool (T::MEMBERNAME is exist)

#define c7typefunc_define_has_member(_M_)				\
    struct has_##_M_##_impl {						\
	template <typename T>						\
	static auto check(T*) -> decltype((&T::_M_, std::true_type{}));	\
									\
	template <typename T>						\
	static auto check(...) -> std::false_type;			\
    };									\
									\
    template <typename T>						\
    struct has_##_M_: public decltype(has_##_M_##_impl::check<T>(nullptr)) {}; \
									\
    template <typename T>						\
    inline constexpr bool has_##_M_##_v = has_##_M_<T>::value


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace typefunc
} // namespace c7


#endif // c7typefunc.hpp
