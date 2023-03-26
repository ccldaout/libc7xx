/*
 * format_r3.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <cstring>
#include <c7format/format_r3.hpp>


namespace c7::format_r3 {


/*----------------------------------------------------------------------------
                               analyzed_format
----------------------------------------------------------------------------*/

thread_local std::unordered_map<std::string, analyzed_format> analyzed_format::s_dic_;
thread_local std::unordered_map<const void*, analyzed_format> analyzed_format::v_dic_;


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::format_r3
