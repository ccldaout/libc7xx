/*
 * c7utils/memory.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_UTILS_MEMORY_HPP_LOADED_
#define C7_UTILS_MEMORY_HPP_LOADED_
#include <c7common.hpp>


#include <memory>
#include <c7result.hpp>	// result_traits


namespace c7 {


/*----------------------------------------------------------------------------
                      unique_ptr factory for malloc/free
----------------------------------------------------------------------------*/

template <typename Ret, typename Arg, Ret(*Free)(Arg*)>
class generic_deleter {
public:
    void operator()(void *p) {
	Free(static_cast<Arg*>(p));
    }
};

using malloc_deleter = generic_deleter<void, void, std::free>;

template <typename T = void>
using unique_cptr = std::unique_ptr<T, malloc_deleter>;

template <typename T = void>
unique_cptr<T> make_unique_cptr(void *p = nullptr)
{
    using item_type = std::conditional_t<std::is_array_v<T>,
					 std::remove_all_extents_t<T>,
					 T>;
    return unique_cptr<T>(static_cast<item_type*>(p), malloc_deleter());
}

template <typename R>
struct result_traits<unique_cptr<R>> {
    static auto init() { return make_unique_cptr<R>(); }
};


} // namespace c7


#endif // c7utils/memory.hpp
