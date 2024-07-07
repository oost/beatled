#include <arpa/inet.h>
#include <cstdio>

#ifndef __APPLE__
#if BYTE_ORDER == BIG_ENDIAN
#define htonll(x) x
#else
#define htonll(x) ((((uint64_t)htonl(x)) << 32) + htonl((x) >> 32))
#endif
#define ntohll(x) htonll(x)
#else
#endif

bool is_big_endian(void) {
  union {
    uint32_t i;
    char c[4];
  } bint = {0x01020304};

  return bint.c[0] == 1;
}

void test_uint64() {
  uint64_t v = 64;
  uint64_t v_n = htonll(v);
  uint64_t v_h = ntohll(v_n);

  printf("Hardware: %llu, Network: %llu, and back: %llu\n", v, v_n, v_h);
}
int main() {
  printf("Big endian: %d\n", is_big_endian());
  test_uint64();
}