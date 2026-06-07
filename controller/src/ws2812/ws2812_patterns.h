#ifndef WS2812_PATTERNS_H
#define WS2812_PATTERNS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*pattern_fn)(uint32_t *stream, size_t len, uint8_t beat_pos,
                           uint32_t beat_count);
typedef void (*pattern_init_fn)();

typedef struct {
  pattern_fn pattern_fn;
  pattern_init_fn pattern_init_fn;
  const char *name;
} pattern;

// size_t num_patterns();
size_t get_pattern_count();

const char *pattern_get_name(uint8_t pattern_idx);

void run_pattern(int pattern_idx, uint32_t *stream, size_t len,
                 uint8_t beat_pos, uint32_t beat_count);

#ifdef __cplusplus
}
#endif

#endif // WS2812_PATTERNS_H
