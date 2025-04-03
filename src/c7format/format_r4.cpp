/*
 * format_r4.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <cstring>
#include <c7format/format_r4.hpp>


namespace c7::format_r4 {


/*----------------------------------------------------------------------------
                               analyzed_format
----------------------------------------------------------------------------*/

std::unordered_map<const void*, analyzed_format> analyzed_format::p_dic_;
volatile int analyzed_format::dic_lock_;


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::format_r4
