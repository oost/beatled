#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "beatled/protocol.h"
#include "programs/programs.h"
#include "programs/utils.h"
#include "ws2812_config.h"
#include "ws2812_patterns.h"

int level = 8;

void pattern_fade_exp(uint32_t *stream, size_t len, uint8_t t, uint32_t beat_count) {
  unsigned int shift = 4;

  unsigned int max = 16; // let's not draw too much current!
  max <<= shift;

  unsigned int slow_t = t / 32;
  slow_t = level;
  slow_t %= max;

  static int error;
  slow_t += error;
  error = slow_t & ((1u << shift) - 1);
  slow_t >>= shift;
  slow_t *= 0x010101;

  for (int i = 0; i < len; ++i) {
    stream[i] = slow_t;
  }
}

// Expanded from the shared protocol header so the server's program list
// and this table can't drift apart.
#define X(id, fn_suffix, display_name) {pattern_##fn_suffix, NULL, display_name},
const pattern _pattern_table[] = {BEATLED_PROGRAM_TABLE(X)};
#undef X

const size_t num_patterns = sizeof(_pattern_table) / sizeof((_pattern_table)[0]);

_Static_assert(sizeof(_pattern_table) / sizeof((_pattern_table)[0]) == BEATLED_PROGRAM_COUNT,
               "BEATLED_PROGRAM_COUNT out of sync with BEATLED_PROGRAM_TABLE");

void get_all_patterns_table(const pattern *pattern_table, size_t *pattern_count) {
  (void)pattern_table;
  *pattern_count = num_patterns;
}

void run_pattern(int pattern_idx, uint32_t *stream, size_t len, uint8_t beat_pos,
                 uint32_t beat_count) {
  int n = (int)num_patterns;
  pattern_idx = ((pattern_idx % n) + n) % n;
  _pattern_table[pattern_idx].pattern_fn(stream, len, beat_pos, beat_count);
}

const char *pattern_get_name(uint8_t pattern_idx) {
  if (pattern_idx >= num_patterns) {
    puts("[ERR] Pattern index out of range");
  }
  return _pattern_table[pattern_idx % num_patterns].name;
}

size_t get_pattern_count() {
  return num_patterns;
}