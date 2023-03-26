/*
 * c7format/helper.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_FORMAT_HELPER_HPP_LOADED__
#define C7_FORMAT_HELPER_HPP_LOADED__
#include <c7common.hpp>


#include <c7format/format_cmn.hpp>
#include <c7nseq/tuple.hpp>


namespace c7 {


// avoid matching with formatter_tag<T*> pattern
template <>
struct formatter_tag<std::string*> {
    typedef formatter_function_tag type;
    static constexpr type value{};
};


} // namespace c7


namespace c7::format_helper {


template <typename T>
static inline decltype(auto) quote_if(const T& arg)
{
    if constexpr (std::is_same_v<T, char*> || std::is_same_v<T, std::string>) {
	return std::quoted(arg);
    } else {
	return arg;
    }
}


template <typename T>
struct format_ident {
    static constexpr const char *name = "?";
};

template <typename T>
static constexpr const char *format_ident_v =
    format_ident<std::remove_const_t<std::remove_reference_t<T>>>::name;


// print_type for several iterable types
template <typename Seq,
	  typename = std::enable_if_t<c7::typefunc::is_iterable_v<Seq>>,
	  typename = std::enable_if_t<
	      !std::is_same_v<decltype(*std::begin(std::declval<Seq>())), char>
	      >>
void print_type(std::ostream& out, const std::string& format, const Seq& arg)
{
    if (std::is_lvalue_reference_v<Seq>) {
	out << '&';
    }
    if (std::is_rvalue_reference_v<Seq>) {
	out << '&';
    }
    out << format_ident_v<Seq>;
    out << "{";
    for (const auto& a: arg) {
	c7::format(out, " %{}", quote_if(a));
    }
    out << " }";
}


} // namespace c7::format_helper


namespace std {


using c7::format_helper::quote_if;


template <int N>
void print_type(std::ostream& out, const std::string& format, const std::string (&sv)[N])
{
    out << "[";
    for (const auto& s: sv) {
	c7::format(out, " %{}", std::quoted(s));
    }
    out << " ]";
}


void print_type(std::ostream& out, const std::string& format, const std::string *s)
{
    c7::format(out, "&[%{}]", *s);
}


template <typename T1, typename T2>
void print_type(std::ostream& out, const std::string& format, const std::pair<T1, T2>& arg)
{
    c7::format(out, "<%{},%{}>", quote_if(arg.first), quote_if(arg.second));
}


template <typename... Ts>
void print_type(std::ostream& out, const std::string& format, const std::tuple<Ts...>& arg)
{
    out << "<(";
    c7::nseq::tuple_for_each(arg, [&out](auto& a) { c7::format(out, "%{},", quote_if(a)); });
    out << ")>";
}


using c7::format_helper::print_type;	// for vector, list, ...


} // namespace std


#endif // c7foramt/helper.hpp
