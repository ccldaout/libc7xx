/*
 * c7app.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef __C7_APP_HPP_LOADED__
#define __C7_APP_HPP_LOADED__


#include <string>


namespace c7::app {


// self program name
extern const std::string& progname;
void set_progname(const std::string& name);


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7::app


#endif // c7app.hpp
