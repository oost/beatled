#ifndef COMMAND__COMMAND_H
#define COMMAND__COMMAND_H

#include <stdint.h>
#include <stdlib.h>

#include "event/event_queue.h"
#include "hello.h"
#include "tempo.h"
#include "time.h"

int handle_event(event_t *event);

// Last successfully-applied PROGRAM push sequence number, or 0 before
// any PROGRAM has arrived. Exposed to feed the QoS block sent on
// TEMPO_REQUEST / STATUS_RESPONSE.
uint16_t program_get_last_applied_seq(void);

#endif // COMMAND__COMMAND_H