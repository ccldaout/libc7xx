/*
 * c7format.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * https://docs.google.com/spreadsheets/d/1PImFGZUZ0JtXuJrrQb8rQ7Zjmh9SqcjTBIe_lkNCl1E/edit#gid=1604237671
 */
#ifndef C7_FORMAT_HPP_LOADED_
#define C7_FORMAT_HPP_LOADED_
#include <c7common.hpp>


#include <c7defer.hpp>	// for compile compatibility


#if !defined(C7_FORMAT_REV)
# define C7_FORMAT_REV 2
#endif


#if C7_FORMAT_REV == 1
#include <c7format/format_r1.hpp>
namespace c7 {
using namespace c7::format_r1;
}
#elif C7_FORMAT_REV == 2
#include <c7format/format_r2.hpp>
namespace c7 {
using namespace c7::format_r2;
}
#elif C7_FORMAT_REV == 3
#include <c7format/format_r3.hpp>
namespace c7 {
using namespace c7::format_r3;
}
#elif C7_FORMAT_REV == 4
#include <c7format/format_r4.hpp>
namespace c7 {
using namespace c7::format_r4;
}
#else
# error "Invalid C7_FORMAT_REV is specified"
}
#endif


#endif // c7format.hpp
