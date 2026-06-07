#include <stdbool.h>
#include <stdint.h>

#include "state_manager/states.h"

static int enter_count = 0;
static int exit_count = 0;

// Stub for push_status_update (POSIX_PORT is defined in tests)
void push_status_update(uint8_t state, bool connected, uint16_t program_id,
                        uint32_t tempo_period_us, uint32_t beat_count,
                        int64_t time_offset) {
  (void)state;
  (void)connected;
  (void)program_id;
  (void)tempo_period_us;
  (void)beat_count;
  (void)time_offset;
}

int get_enter_count(void) { return enter_count; }
int get_exit_count(void) { return exit_count; }
void reset_stub_counters(void) {
  enter_count = 0;
  exit_count = 0;
}

int enter_started_state() {
  enter_count++;
  return 0;
}
int exit_started_state() {
  exit_count++;
  return 0;
}

int enter_initialized_state() {
  enter_count++;
  return 0;
}
int exit_initialized_state() {
  exit_count++;
  return 0;
}

int enter_registered_state() {
  enter_count++;
  return 0;
}
int exit_registered_state() {
  exit_count++;
  return 0;
}

int enter_time_synced_state() {
  enter_count++;
  return 0;
}
int exit_time_synced_state() {
  exit_count++;
  return 0;
}

int enter_tempo_synced_state() {
  enter_count++;
  return 0;
}
int exit_tempo_synced_state() {
  exit_count++;
  return 0;
}
