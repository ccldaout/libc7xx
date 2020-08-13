/*
 * c7common.cpp
 *
 * Copyright (c) 2019 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include "c7common.hpp"


namespace c7 {


char __dummy[2];
void * const eof_marker = static_cast<void*>(&__dummy[0]);
void * const abort_marker = static_cast<void*>(&__dummy[1]);

struct drop drop;


} // namespace c7
