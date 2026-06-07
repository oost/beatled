#ifndef COMMAND__COMMAND_H
#define COMMAND__COMMAND_H

#include <stdlib.h>

#include "event/event_queue.h"
#include "hello.h"
#include "tempo.h"
#include "time.h"

int handle_event(event_t *event);

#endif // COMMAND__COMMAND_H