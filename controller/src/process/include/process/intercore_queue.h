#ifndef INTERCORE_QUEUE_H
#define INTERCORE_QUEUE_H

#include <stdint.h>

#include "hal/queue.h"

extern hal_queue_handle_t intercore_command_queue;

typedef enum {
  intercore_time_ref_update = 0,
  intercore_tempo_update,
  intercore_program_update
} intercore_message_type_t;

// `message_type` is a bitmask so one queue entry can coalesce several
// updates. Build and test masks with these flags, not raw shifts.
#define INTERCORE_FLAG(type) (0x01u << (type))
#define INTERCORE_FLAG_TIME_REF_UPDATE INTERCORE_FLAG(intercore_time_ref_update)
#define INTERCORE_FLAG_TEMPO_UPDATE INTERCORE_FLAG(intercore_tempo_update)
#define INTERCORE_FLAG_PROGRAM_UPDATE INTERCORE_FLAG(intercore_program_update)

typedef struct {
  uint32_t message_type; // OR of INTERCORE_FLAG_* bits
} intercore_message_t;

#endif // INTERCORE_QUEUE_H