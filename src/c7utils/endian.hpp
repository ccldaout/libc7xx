/*
 * c7utils/endian.hpp
 *
 * Copyright (c) 2020 ccldaout@gmail.com
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * Google spreadsheets:
 * (Nothing)
 */
#ifndef C7_UTILS_ENDIAN_HPP_LOADED_
#define C7_UTILS_ENDIAN_HPP_LOADED_
#include <c7common.hpp>


namespace c7::endian {


/*----------------------------------------------------------------------------
                                reverse endian
----------------------------------------------------------------------------*/

#if defined(__x86_64__)

static inline void reverse(uint64_t& u) { __asm__("bswapq %0":"+r"(u)); }
static inline void reverse( int64_t& u) { __asm__("bswapq %0":"+r"(u)); }
static inline void reverse(uint32_t& u) { __asm__("bswapl %0":"+r"(u)); }
static inline void reverse( int32_t& u) { __asm__("bswapl %0":"+r"(u)); }
static inline void reverse(uint16_t& u) { __asm__("rorw $8,%0":"+r"(u)); }
static inline void reverse( int16_t& u) { __asm__("rorw $8,%0":"+r"(u)); }

#elif defined(__aarch64__)

static inline void reverse(uint64_t& u) { __asm__("rev %0,%0":"+r"(u)); }
static inline void reverse( int64_t& u) { __asm__("rev %0,%0":"+r"(u)); }
static inline void reverse(uint32_t& u) { __asm__("rev32 %0,%0":"+r"(u)); }
static inline void reverse( int32_t& u) { __asm__("rev32 %0,%0":"+r"(u)); }
static inline void reverse(uint16_t& u) { __asm__("rev16 %0,%0":"+r"(u)); }
static inline void reverse( int16_t& u) { __asm__("rev16 %0,%0":"+r"(u)); }

#else

static inline void reverse(uint64_t& u) {
    uint64_t m = 0x00ff00ff00ff00ffUL;
    u = ((u & m) << 8) | ((u & (m << 8)) >> 8);
    m = 0x0000ffff0000ffffUL;
    u = ((u & m) << 16) | ((u & (m << 16)) >> 16);
    u = (u << 32) | (u >> 32);
}

static inline void reverse(uint32_t& u) {
    uint32_t m = 0x00ff00ffUL;
    u = ((u & m) << 8) | ((u & (m << 8)) >> 8);
    u = (u << 16) | (u >> 16);
}

static inline void reverse(uint16_t& u) {
    u = (u << 8) | (u >> 8);
}
static inline void reverse( int64_t& u) {
    reverse(reinterpret_cast<uint64_t&>(u));
}
static inline void reverse( int32_t& u) {
    reverse(reinterpret_cast<uint32_t&>(u));
}
static inline void reverse( int16_t& u) {
    reverse(reinterpret_cast<uint16_t&>(u));
}

#endif

static inline void reverse(uint8_t) {}
static inline void reverse( int8_t) {}
template <typename T> static inline T reverse_to(T u) { reverse(u); return u; }


} // namespace c7::endian


#endif // c7utils/endian.hpp
