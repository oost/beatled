#ifndef INTERCORE_QUEUE_H
#define INTERCORE_QUEUE_H

#include "hal/queue.h"

extern hal_queue_handle_t intercore_command_queue;

typedef enum {
  intercore_time_ref_update = 0,
  intercore_tempo_update,
  intercore_program_update
} intercore_message_type_t;

typedef struct {
  intercore_message_type_t message_type;
} intercore_message_t;

#endif // INTERCORE_QUEUE_H