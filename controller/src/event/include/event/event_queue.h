#ifndef EVENT__EVENT_QUEUE_H
#define EVENT__EVENT_QUEUE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "event.h"

// Network ports call this on receive. rx_time_us is the local monotonic
// timestamp captured at packet arrival; it travels through the event and
// is used by the time-sync algorithm as dest_time, replacing the
// dequeue-time that used to leak event-loop scheduling jitter.
int add_payload_to_event_queue(void *buffer_payload, size_t size,
                               uint64_t rx_time_us);

void event_queue_init();

bool event_queue_add_message(event_type_t event_type, void *event_data,
                             size_t data_length);

// Variant that lets the producer override the event's timestamp. Used by
// the receive path so dest_time reflects packet arrival.
bool event_queue_add_message_at(event_type_t event_type, void *event_data,
                                size_t data_length, uint64_t time_us);

void event_queue_pop_message_blocking(event_t *event);

#endif // EVENT__EVENT_QUEUE_H