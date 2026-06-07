#ifndef HAL__UTILS__NETWORK_H
#define HAL__UTILS__NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(PICO_PORT) || defined(ESP32_PORT)
#include <lwip/def.h>
#include <stdint.h>

// Inline functions instead of macros so the 64→32 narrowing inside the
// little-endian byteswap is explicit and the compiler doesn't warn about
// it. (The narrowing is intentional — `htonl(low)` only ever sees the
// low 32 bits — but GCC's -Woverflow flagged the implicit cast.)
//
// On a big-endian target, network byte order == host byte order, so the
// function is a no-op. On a little-endian target, swap the halves.
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || defined(__BIG_ENDIAN__) ||            \
    defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(_MIBSEB) ||    \
    defined(__MIBSEB) || defined(__MIBSEB__)
static inline uint64_t pico_htonll(uint64_t x) {
  return x;
}
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || defined(__LITTLE_ENDIAN__) ||    \
    defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) ||    \
    defined(__MIPSEL) || defined(__MIPSEL__) || defined(__riscv)
static inline uint64_t pico_htonll(uint64_t x) {
  return ((uint64_t)htonl((uint32_t)x) << 32) | htonl((uint32_t)(x >> 32));
}
#else
#error "I don't know what architecture this is!"
#endif

static inline uint64_t pico_ntohll(uint64_t x) {
  return pico_htonll(x);
}

#define htonll(x) pico_htonll(x)
#define ntohll(x) pico_ntohll(x)

#else

#include <arpa/inet.h>

#endif

#ifdef __cplusplus
}
#endif
#endif // HAL__UTILS__NETWORK_H
