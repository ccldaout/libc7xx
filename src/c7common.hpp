/*
 * c7common.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
#ifndef C7_COMMON_HPP_LOADED__
#define C7_COMMON_HPP_LOADED__


#include <limits.h>
#include <stddef.h>
#include <sys/types.h>


/*----------------------------------------------------------------------------
                          definition from c7config.h
----------------------------------------------------------------------------*/

#if (defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE < 200809L))
# undef _POSIX_C_SOURCE
#endif
#if !defined(_POSIX_C_SOURCE)
# define _POSIX_C_SOURCE 200809L
#endif

#undef  _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED 1

#undef  _XOPEN_SOURCE
#define _XOPEN_SOURCE 1

#undef  _GNU_SOURCE
#define _GNU_SOURCE 1


/*----------------------------------------------------------------------------
                          definition from c7types.h
----------------------------------------------------------------------------*/

#define C7_TIME_S_us	((c7::usec_t)1'000'000)

#define C7_SYSERR      	(-1)
#define C7_SYSOK	(0)
#define C7_SYSNOERR	C7_SYSOK

// log level: 0..7
#define C7_LOG_MIN	(0U)
#define C7_LOG_MAX	(7U)
#define C7_LOG_ALW	C7_LOG_MIN	// always
#define C7_LOG_ERR	(1U)
#define C7_LOG_WRN	(2U)
#define C7_LOG_INF	(3U)
#define C7_LOG_BRF	(4U)		// more information briefly
#define C7_LOG_DTL	(5U)		// more information in detail
#define C7_LOG_TRC	(6U)		// trace (short message, many log)
#define C7_LOG_DBG	C7_LOG_MAX

#define c7_align(_v, _power_of_2)	(1 + (((_v)-1) | ((_power_of_2)-1)))
#define c7_numberof(_a)			((ssize_t)(sizeof(_a)/sizeof((_a)[0])))


namespace c7 {


/*----------------------------------------------------------------------------
                              basic definitions
----------------------------------------------------------------------------*/

// micorsecond
typedef int64_t usec_t;

// left hand operand for dummy assignment to suppress unused-warning
struct drop {
    template <typename T>
    void operator=(const T&) {}
};
extern struct drop drop;

// communication status
enum class com_status {
    OK, CLOSED, TIMEOUT, ERROR, ABORT,
};

// format traits primary template
template <typename T>
struct format_traits {};


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/


} // namespace c7


#endif // c7common.hpp
