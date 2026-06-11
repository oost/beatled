/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config/constants.h"
#include "hal/registry.h"
#include "hal/ws2812.h"
#include "process/intercore_queue.h"
#include "state_manager/state_manager.h"
#ifdef POSIX_PORT
#include "hal/startup.h"
#else
#include "pico/time.h"
#endif
#include "programs/utils.h"
#include "ws2812/ws2812.h"
#include "ws2812_config.h"
#include "ws2812_patterns.h"

#ifdef PICO_PORT
#define LED_SELF_TEST_STEP_MS 1200
#define LED_SELF_TEST_BRIGHTNESS 125

static void led_self_test_fill(uint32_t *colors, uint32_t value, const char *label) {
  for (size_t p = 0; p < NUM_PIXELS; p++)
    colors[p] = value;
  output_strings_dma(colors);
  printf("[INIT] LED self-test: %s\n", label);
  sleep_ms(LED_SELF_TEST_STEP_MS);
}

static void led_self_test(void) {
  uint32_t colors[NUM_PIXELS];

  // Hold each colour ~1.2s so it's impossible to miss even glancing at
  // the strip. The R/G/B/W sweep also tells you whether the channel
  // ordering / data line is healthy.
  puts("[INIT] LED self-test: starting (~6s of full-brightness colours)");
  led_self_test_fill(colors, rgb_u32(LED_SELF_TEST_BRIGHTNESS, 0, 0), "RED");
  led_self_test_fill(colors, rgb_u32(0, LED_SELF_TEST_BRIGHTNESS, 0), "GREEN");
  led_self_test_fill(colors, rgb_u32(0, 0, LED_SELF_TEST_BRIGHTNESS), "BLUE");
  led_self_test_fill(
      colors, rgb_u32(LED_SELF_TEST_BRIGHTNESS, LED_SELF_TEST_BRIGHTNESS, LED_SELF_TEST_BRIGHTNESS),
      "WHITE");
  led_self_test_fill(colors, 0, "off");
  puts("[INIT] LED self-test: done");
}
#else
static void led_self_test(void) {
  // No real strip on the POSIX simulator; skip the visible flash sequence.
}
#endif

uint32_t _cycle_idx = 0;
uint64_t _time_ref = 0;
uint64_t _last_beat_time = 0;
uint64_t _next_beat_time = 0;
uint64_t _tempo_period_us = 0;
// Beat count of the beat that fires at _next_beat_time. The count rendered
// for the beat in progress is _next_beat_count - 1; deriving it from the
// grid keeps it consistent with the beat fraction on every frame.
uint32_t _next_beat_count = 0;
uint8_t _program_id = 0;

void led_init() {
  ws2812_init(NUM_PIXELS, WS2812_PIN, 800000, IS_RGBW);
  puts("[INIT] LED manager initialized");
  led_self_test();
}

void led_set_random_pattern() {
  if (registry_try_lock_mutex()) {
    // registry_set_program(rand() % get_pattern_count());
    registry.program_id = 0;
    registry_unlock_mutex();
  }
}

uint8_t calculate_beat_fraction(uint64_t current_time, uint64_t last_time, uint64_t next_time) {
  return scale8(current_time - last_time, next_time - last_time);
}

uint8_t scale8(uint64_t value, uint64_t range) {
  if (range == 0) {
    return 0;
  }
  if (value >= range) {
    return 255;
  }
  uint8_t result = 0;
  for (int i = 0; i < 8; i++) {
    range = range >> 1;

    if (value >= range) {
      result += 1 << (7 - i);
      value -= range;
    }
  }

  return result;
}

void update_tempo(intercore_message_t *ic_message) {
  registry_lock_mutex();

  // printf("Message type %d\n", ic_message->message_type);
  if (ic_message->message_type & INTERCORE_FLAG_TIME_REF_UPDATE) {
    uint64_t ref = registry.next_beat_time_ref;
    uint32_t count_at_ref = registry.beat_count;
    uint64_t now = time_us_64();

#if BEATLED_VERBOSE_LOG
    printf("[TEMPO] Time ref update: %llu -> %llu (shift=%lld)\n", _next_beat_time, ref,
           (int64_t)(ref - _next_beat_time));
#endif

    if (_tempo_period_us > 0) {
      // The announced beat may already have passed (late delivery) or be
      // more than one beat away (early delivery racing the local wrap).
      // Roll it by whole periods, count in step, so it always names the
      // next upcoming boundary — a re-anchor can then nudge the phase by
      // the clock-sync error only, never jump the grid by a full beat.
      while (ref <= now) {
        ref += _tempo_period_us;
        count_at_ref++;
      }
      while (ref > now + _tempo_period_us) {
        ref -= _tempo_period_us;
        count_at_ref--;
      }
      _last_beat_time = ref - _tempo_period_us;
    } else {
      // No tempo yet; led_update stays idle until one arrives.
      _last_beat_time = ref;
    }
    _time_ref = registry.next_beat_time_ref;
    _next_beat_time = ref;
    _next_beat_count = count_at_ref;
  }

  if (ic_message->message_type & INTERCORE_FLAG_TEMPO_UPDATE) {
    _tempo_period_us = registry.tempo_period_us;

    // Initialize time_ref on first tempo update if not set yet
    if (_time_ref == 0 && _tempo_period_us > 0) {
      _time_ref = time_us_64();
      _last_beat_time = _time_ref;
      _next_beat_time = _time_ref + _tempo_period_us;
      _next_beat_count = 1;
    }

#if BEATLED_VERBOSE_LOG
    printf("[TEMPO] Period=%llu us (%.1f BPM)\n", _tempo_period_us,
           _tempo_period_us > 0 ? 60000000.0 / _tempo_period_us : 0.0);
#endif
  }

  if (ic_message->message_type & INTERCORE_FLAG_PROGRAM_UPDATE) {
    uint8_t new_program_id = registry.program_id;
    if (new_program_id != _program_id) {
      _program_id = new_program_id;
      printf("[LED] Program update: id=%u\n", _program_id);
    }
  }

  registry_unlock_mutex();
}

uint32_t led_update(void) {
  const uint32_t frame_us = (uint32_t)LED_CORE_SLEEP_MS * 1000u;

  // Snapshot shared state under registry lock to prevent torn reads from core0
  registry_lock_mutex();
  uint64_t time_ref = _time_ref;
  uint64_t tempo_period_us = _tempo_period_us;
  uint64_t last_beat_time = _last_beat_time;
  uint64_t next_beat_time = _next_beat_time;
  uint32_t next_beat_count = _next_beat_count;
  uint8_t program_id = _program_id;
  int64_t time_offset = (int64_t)registry.time_offset;
  registry_unlock_mutex();

  if (time_ref == 0 || tempo_period_us == 0) {
    return frame_us;
  }

  static uint32_t colors[2][NUM_PIXELS];
  static unsigned int current_stream = 0;

  uint64_t current_time = time_us_64();

  // Advance the beat grid (using local copies). The count moves with the
  // boundary, before rendering, so the pattern never sees a fresh beat
  // fraction paired with the previous beat's count.
  while (next_beat_time < current_time) {
#if BEATLED_VERBOSE_LOG
    puts("[LED] Advancing next beat time");
#endif
    last_beat_time = next_beat_time;
    next_beat_time += tempo_period_us;
    next_beat_count++;
  }

  uint8_t beat_frac = calculate_beat_fraction(current_time, last_beat_time, next_beat_time);

  // Count of the beat in progress: one behind the upcoming boundary's count.
  uint32_t beat_count = next_beat_count - 1;

#if BEATLED_VERBOSE_LOG
  printf("[BEATFRAC] beat_frac=%u (current_time=%llu last_beat_time=%llu "
         "next_beat_time=%llu beat=%" PRIu32 ")\n",
         beat_frac, current_time, last_beat_time, next_beat_time, beat_count);
#endif

  run_pattern(program_id, colors[current_stream], NUM_PIXELS, beat_frac, beat_count);

  output_strings_dma(colors[current_stream]);
  current_stream ^= 1;

  // Update status ~10x per second (every 10 LED cycles at 100Hz)
  if (_cycle_idx % 10 == 0) {
#ifdef POSIX_PORT
    push_status_update(state_manager_get_state(), state_manager_get_state() >= STATE_REGISTERED,
                       program_id, (uint32_t)tempo_period_us, beat_count, time_offset);
#endif
  }

  // Verbose logging every 1000 cycles (~10 seconds)
  if (_cycle_idx % 1000 == 0) {
#if BEATLED_VERBOSE_LOG
    printf("[LED] cycle=%" PRIu32 " program=%u beat_frac=%.3f tempo=%llu us (%.1f BPM) "
           "beat=%" PRIu32 "\n",
           _cycle_idx, program_id, (float)beat_frac / UINT8_MAX, tempo_period_us,
           tempo_period_us > 0 ? 60000000.0 / tempo_period_us : 0.0, beat_count);
#endif
  }

  _cycle_idx++;

  // Write back modified state under lock
  registry_lock_mutex();
  _last_beat_time = last_beat_time;
  _next_beat_time = next_beat_time;
  _next_beat_count = next_beat_count;
  registry_unlock_mutex();

  // Wake for the beat boundary if it falls before the next regular frame so
  // the onset frame renders right at the beat on every controller.
  uint64_t after = time_us_64();
  if (next_beat_time > after) {
    uint64_t remaining = next_beat_time - after;
    if (remaining < frame_us) {
      return (uint32_t)remaining;
    }
  }
  return frame_us;
}
