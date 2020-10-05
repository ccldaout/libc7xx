/*
 * c7delegate.cpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */


#include <c7delegate.hpp>


namespace c7 {


// initialize static data member.
std::atomic<unsigned long> delegate_base::id::id_counter_;	// in some cpp


} // namespace c7
