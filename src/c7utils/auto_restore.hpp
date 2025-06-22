/*
 * c7utils/auto_restore.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_UTILS_AUTO_RESTORE_HPP_LOADED_
#define C7_UTILS_AUTO_RESTORE_HPP_LOADED_
#include <c7common.hpp>


#include <type_traits>


namespace c7 {


template <typename... Args>
class auto_restore_impl;

template <typename Arg>
class auto_restore_impl<Arg> {
private:
    using value_type = std::remove_reference_t<Arg>;
    value_type& ref_;
    value_type save_;

public:
    explicit auto_restore_impl(value_type& ref):
	ref_(ref), save_(ref) {
    }

    ~auto_restore_impl() {
	ref_ = save_;
    }
};

template <typename Arg, typename... Args>
class auto_restore_impl<Arg, Args...>: public auto_restore_impl<Args...> {
private:
    using value_type = std::remove_reference_t<Arg>;
    value_type& ref_;
    value_type save_;

public:
    explicit auto_restore_impl(value_type& ref, Args&... args):
	auto_restore_impl<Args...>(args...),
	ref_(ref), save_(ref) {
    }

    ~auto_restore_impl() {
	ref_ = save_;
    }
};

template <typename... Args>
auto auto_restore(Args&... args)
{
    return auto_restore_impl<Args...>(args...);
}


} // namespace c7


#if !defined(C7_UTILS_AUTO_RESTORE_NO_MACRO)
# define c7auto_restore(...) if (auto c7auto_restore_vars_ = c7::auto_restore(__VA_ARGS__); true)
#endif


#endif // c7utils/auto_restore.hpp
