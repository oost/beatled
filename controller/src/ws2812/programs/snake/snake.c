#include "snake.h"

#include "../utils.h"

// Snake travels one full lap of the ring every SNAKE_BEATS_PER_LOOP beats.
#define SNAKE_BEATS_PER_LOOP 4u
// Body length as a fraction of the ring (denominator): 2 -> half the ring.
#define SNAKE_TAIL_DIVISOR 3u
// Perceptual brightness floor between beats so the body never fully
// disappears (0-255, pre-gamma: keep above ~30 or gamma8[] rounds it to 0).
#define SNAKE_BEAT_FLOOR 70u
// Brightest channel value this program may emit (0-255); bounds current draw.
#define SNAKE_MAX_BRIGHTNESS 255u

void pattern_snakes_init() {}

void pattern_snakes(uint32_t *stream, size_t len, uint8_t t, uint32_t beat_count) {
  if (len == 0) {
    return;
  }

  // Continuous phase within one lap, in 1/256-beat units: 0 .. N*256-1. Using
  // beat_count keeps the head moving smoothly across beat boundaries instead of
  // resetting every beat.
  uint32_t loop_phase = ((uint32_t)(beat_count % SNAKE_BEATS_PER_LOOP) << 8) + t;

  // Head position in 1/256-LED fixed point, wrapping once per lap. Over N beats
  // loop_phase spans N*256, so the head travels exactly `len` LEDs -> one lap.
  uint32_t ring_q8 = (uint32_t)len << 8;
  uint32_t head_q8 = (loop_phase * (uint32_t)len) / SNAKE_BEATS_PER_LOOP;
  head_q8 %= ring_q8;

  // Body length in 1/256-LED units (at least one LED).
  uint32_t tail_q8 = ring_q8 / SNAKE_TAIL_DIVISOR;
  if (tail_q8 == 0) {
    tail_q8 = 256;
  }

  // Whole-body brightness envelope: peaks at the beat (t=0) and decays toward
  // the next beat, never below SNAKE_BEAT_FLOOR so the snake stays visible as
  // it travels.
  uint8_t beat_env =
      SNAKE_BEAT_FLOOR + (uint8_t)(((255u - SNAKE_BEAT_FLOOR) * beat_intensity_quadratic(t)) >> 8);

  // Hue advances each beat so successive laps differ; a gentle gradient runs
  // along the body for depth.
  uint8_t head_hue = (uint8_t)(beat_count * 32u);

  for (size_t i = 0; i < len; ++i) {
    // How far this pixel sits behind the head, walking backwards round the
    // ring, in 1/256-LED units. 0 == exactly at the head.
    uint32_t pixel_q8 = (uint32_t)i << 8;
    uint32_t dist_q8 = (head_q8 + ring_q8 - pixel_q8) % ring_q8;

    if (dist_q8 >= tail_q8) {
      stream[i] = 0; // outside the body
      continue;
    }

    // Spatial fade: bright at the head, dark at the tail tip.
    uint32_t tail_fade = (tail_q8 - dist_q8) * 255u / tail_q8; // 0..255
    uint8_t value = (uint8_t)((tail_fade * beat_env) >> 8);

    uint8_t hue = head_hue + (uint8_t)(dist_q8 * 48u / tail_q8);
    // Full-brightness colour scaled down, rather than HSV at a low value,
    // so the tail fades without hue-quantization flicker. The combined
    // fade is gamma-corrected and capped at the program ceiling first.
    stream[i] = color_scale(convert_hsv_to_rgb(hue, 255, 255),
                            brightness_apply(value, SNAKE_MAX_BRIGHTNESS));
  }
}
