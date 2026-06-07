// #include <pico/sync.h>
#include <stdio.h>
#include <stdlib.h>

#include "config/constants.h"
#include "event/event_queue.h"
#include "hal/registry.h"
#include "hal/unique_id.h"
#include "state_manager/state_manager.h"
#include "state_manager/states.h"
#ifdef POSIX_PORT
extern void push_status_update(uint8_t state, bool connected,
                               uint16_t program_id, uint32_t tempo_period_us,
                               uint32_t beat_count, int64_t time_offset);
#endif

typedef struct state_manager_internal_state {
  state_manager_state_t current_state;
  int64_t last_tempo_sync_time;
} state_manager_internal_state_t;

state_manager_internal_state_t internal_state = {.current_state = 0,
                                                 .last_tempo_sync_time = 0};
exit_state_fn exit_current_state;

// Each element in the matrix is a mask determining the states that can be
// transitioned to from a given state.
uint16_t transition_matrix[] = {
    0x01 << STATE_STARTED,      // STATE_UNKNOWN
    0x01 << STATE_INITIALIZED,  // STATE_STARTED
    0x01 << STATE_REGISTERED,   // STATE_INITIALIZED
    0x01 << STATE_TIME_SYNCED,  // STATE_REGISTERED
    0x01 << STATE_TEMPO_SYNCED, // STATE_TIME_SYNCED
    (0x01 << STATE_TIME_SYNCED) |
        (0x01 << STATE_TEMPO_SYNCED) // STATE_TEMPO_SYNCED
};

const char *state_name(state_manager_state_t s) {
  switch (s) {
  case STATE_UNKNOWN:
    return "UNKNOWN";
  case STATE_STARTED:
    return "STARTED";
  case STATE_INITIALIZED:
    return "INITIALIZED";
  case STATE_REGISTERED:
    return "REGISTERED";
  case STATE_TIME_SYNCED:
    return "TIME_SYNCED";
  case STATE_TEMPO_SYNCED:
    return "TEMPO_SYNCED";
  default:
    return "???";
  }
}

void state_manager_init() {}

state_manager_state_t state_manager_get_state() {
  return internal_state.current_state;
}

static const size_t transition_matrix_size =
    sizeof(transition_matrix) / sizeof(transition_matrix[0]);

int transition_state(state_manager_state_t new_state) {
  state_manager_state_t old_state = internal_state.current_state;

  if (new_state >= transition_matrix_size ||
      old_state >= transition_matrix_size) {
    printf("[STATE] Invalid state: old=%s new=%s (max=%zu)\n",
           state_name(old_state), state_name(new_state),
           transition_matrix_size - 1);
    return 2;
  }

  if (old_state == new_state) {
    if (new_state != STATE_TEMPO_SYNCED) {
      printf("[STATE] Transitioning to the same state (%s)... Noop\n",
             state_name(old_state));
      // Should this be an error? For now, just ignore it. It can happen for
      // example when receiving a "next beat" command while already in the tempo
      // synced state, which issues a transition to the same state as a way to
      // re-sync the tempo.
      return 1;
    }
    printf("[STATE] Re-entering state %s (re-sync)\n", state_name(old_state));
  }

  int err = 0;

  if (((transition_matrix[internal_state.current_state] &
        (0x01 << new_state)) == 0)) {
    printf("[STATE] Transition not allowed from %s to %s\n",
           state_name(old_state), state_name(new_state));
    return 2;
  }

  printf("[STATE] %s -> %s\n", state_name(old_state), state_name(new_state));

  if (exit_current_state) {
    err = exit_current_state();
    exit_current_state = NULL;
  } else if (old_state) {
    puts("[STATE] No exit handler for previous state");
  }

  internal_state.current_state = new_state;

#ifdef POSIX_PORT
  // Read current registry values to preserve tempo/program info in HUD
  registry_lock_mutex();
  uint16_t program_id = registry.program_id;
  uint32_t tempo_period_us = registry.tempo_period_us;
  uint32_t beat_count = registry.beat_count;
  int64_t time_offset = (int64_t)registry.time_offset;
  registry_unlock_mutex();

  push_status_update(new_state, new_state >= STATE_REGISTERED, program_id,
                     tempo_period_us, beat_count, time_offset);
#endif

  switch (new_state) {
  case STATE_STARTED:
    enter_started_state();
    exit_current_state = &exit_started_state;
    break;
  case STATE_INITIALIZED:
    enter_initialized_state();
    exit_current_state = &exit_initialized_state;
    break;

  case STATE_REGISTERED:
    enter_registered_state();
    exit_current_state = &exit_registered_state;
    break;

  case STATE_TIME_SYNCED:
    enter_time_synced_state();
    exit_current_state = &exit_time_synced_state;
    break;

  case STATE_TEMPO_SYNCED:
    enter_tempo_synced_state();
    exit_current_state = &exit_tempo_synced_state;
    break;

  default:
    printf("[STATE] Unknown state: %s (%d)\n", state_name(new_state),
           new_state);
  }

  return err;
}

int state_manager_set_state(state_manager_state_t state) {
  return transition_state(state);
}

bool schedule_state_transition(state_manager_state_t next_state) {
  state_event_t *state_event = (state_event_t *)malloc(sizeof(state_event_t));
  if (!state_event) {
    BEATLED_FATAL("Failed to allocate state event");
    return false;
  }
  state_event->next_state = next_state;
  return event_queue_add_message(event_state_transition, state_event,
                                 sizeof(state_event_t));
}
