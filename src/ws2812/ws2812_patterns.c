#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "programs/programs.h"
#include "programs/utils.h"
#include "ws2812_config.h"
#include "ws2812_patterns.h"

int level = 8;

void pattern_fade_exp(uint32_t *stream, size_t len, uint8_t t,
                      uint32_t beat_count) {
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

const pattern _pattern_table[] = {
    {pattern_snakes, NULL, "Snakes!"},
    {pattern_random, NULL, "Random data"},
    {pattern_sparkle, NULL, "Sparkles"},
    {pattern_greys, NULL, "Greys"},
    {pattern_drops, NULL, "Drops"},
    {pattern_solid, NULL, "Solid!"},
    {pattern_fade_grey, NULL, "Fade"},
    {pattern_fade_color, NULL, "Fade Colors"},
};

const size_t num_patterns =
    sizeof(_pattern_table) / sizeof((_pattern_table)[0]);

void get_all_patterns_table(const pattern *pattern_table,
                            size_t *pattern_count) {
  (void)pattern_table;
  *pattern_count = num_patterns;
}

void run_pattern(int pattern_idx, uint32_t *stream, size_t len,
                 uint8_t beat_pos, uint32_t beat_count) {
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

size_t get_pattern_count() { return num_patterns; }