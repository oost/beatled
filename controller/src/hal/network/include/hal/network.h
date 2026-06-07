#ifndef HAL__UTILS__NETWORK_H
#define HAL__UTILS__NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(PICO_PORT) || defined(ESP32_PORT)
#include <lwip/def.h>

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN ||                   \
    defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) ||   \
    defined(__AARCH64EB__) || defined(_MIBSEB) || defined(__MIBSEB) ||         \
    defined(__MIBSEB__)
#define pico_htonll(x) ((u64_t)(x))
#define pico_ntohll(x) ((u64_t)(x))
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN ||              \
    defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) ||                        \
    defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) ||      \
    defined(__MIPSEL) || defined(__MIPSEL__) || defined(__riscv)
#define pico_htonll(x) ((((uint64_t)htonl(x)) << 32) + htonl((x) >> 32))
#define pico_ntohll(x) pico_htonll(x)
#else
#error "I don't know what architecture this is!"
#endif

#define htonll(x) pico_htonll(x)
#define ntohll(x) pico_ntohll(x)

#else

#include <arpa/inet.h>

#endif

#ifdef __cplusplus
}
#endif
#endif // HAL__UTILS__NETWORK_H
