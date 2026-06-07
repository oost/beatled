#ifndef EVENT__EVENT_LOOP_H_
#define EVENT__EVENT_LOOP_H_

#include <stdint.h>

#include "event/event_queue.h"

typedef int (*handle_event_fn)(event_t *);

void run_event_loop(handle_event_fn handle_event);

#endif // EVENT__EVENT_LOOP_H_