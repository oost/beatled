#include "off.h"

void pattern_off(uint32_t *stream, size_t len, uint8_t t, uint32_t beat_count) {
  (void)t;
  (void)beat_count;
  for (size_t i = 0; i < len; ++i) {
    stream[i] = 0;
  }
}
