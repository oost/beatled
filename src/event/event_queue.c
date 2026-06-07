// #include <pico/util/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "event/event_queue.h"
#include "hal/queue.h"
#include "hal/time.h"

#define MAX_QUEUE_COUNT 64

// static queue_t event_queue;

hal_queue_handle_t event_queue_ptr;

int add_payload_to_event_queue(void *buffer_payload, size_t size,
                               uint64_t rx_time_us) {
  if (event_queue_add_message_at(event_server_message, buffer_payload, size,
                                 rx_time_us)) {
    return 0;
  }
  return 1;
}

void event_queue_init() {
  event_queue_ptr = hal_queue_init(sizeof(event_t), MAX_QUEUE_COUNT);
}

bool event_queue_add_message(event_type_t event_type, void *event_data,
                             size_t data_length) {
  return event_queue_add_message_at(event_type, event_data, data_length,
                                    time_us_64());
}

bool event_queue_add_message_at(event_type_t event_type, void *event_data,
                                size_t data_length, uint64_t time_us) {
  event_t event;
  event.event_type = event_type;
  event.time = time_us;
  event.data = event_data;
  event.data_length = data_length;

  return hal_queue_add_message(event_queue_ptr, &event);
}

void event_queue_pop_message_blocking(event_t *event) {
  hal_queue_pop_message_blocking(event_queue_ptr, event);
}