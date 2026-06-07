#ifndef HAL__REGISTRY_H
#define HAL__REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct registry {
  uint64_t time_offset;
  uint64_t time_offset_update_timestamp;
  uint64_t last_beat_time_ref;
  uint64_t next_beat_time_ref;
  uint64_t update_timestamp;
  uint64_t tempo_time_update_timestamp;
  uint32_t tempo_period_us;
  uint32_t beat_count;
  uint16_t program_id;
} registry_t;

extern registry_t registry;

void registry_init();

void registry_lock_mutex();

void registry_unlock_mutex();

bool registry_try_lock_mutex();

#ifdef __cplusplus
}
#endif

#endif // HAL__REGISTRY_H