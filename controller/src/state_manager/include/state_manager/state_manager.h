#ifndef STATE_MANAGER__STATE_MANAGER_H
#define STATE_MANAGER__STATE_MANAGER_H

#include "hal/time.h"
#include "hal/unique_id.h"

typedef enum {
  STATE_UNKNOWN = 0,
  STATE_STARTED,
  STATE_INITIALIZED,
  STATE_REGISTERED,
  STATE_TIME_SYNCED,
  STATE_TEMPO_SYNCED
} state_manager_state_t;

typedef struct {
  state_manager_state_t next_state;
} state_event_t;

typedef int (*enter_state_fn)();
typedef int (*exit_state_fn)();

void state_manager_init();
int state_manager_set_state(state_manager_state_t state);
state_manager_state_t state_manager_get_state();
const char *state_name(state_manager_state_t state);

bool schedule_state_transition(state_manager_state_t next_state);

#endif // STATE_MANAGER__STATE_MANAGER_H