#include <catch2/catch_test_macros.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#include "event/event_queue.h"
#include "state_manager/state_manager.h"

// Stub counter accessors (defined in state_stubs.c)
int get_enter_count(void);
int get_exit_count(void);
void reset_stub_counters(void);

// From state_manager.c — these have external linkage so we can reset them
typedef struct state_manager_internal_state {
  state_manager_state_t current_state;
  int64_t last_tempo_sync_time;
} state_manager_internal_state_t;

extern state_manager_internal_state_t internal_state;
extern exit_state_fn exit_current_state;

#ifdef __cplusplus
}
#endif

static void reset_state_machine() {
  internal_state.current_state = STATE_UNKNOWN;
  internal_state.last_tempo_sync_time = 0;
  exit_current_state = NULL;
  reset_stub_counters();
  event_queue_init();
}

TEST_CASE("State machine transitions", "[state_manager]") {
  reset_state_machine();

  SECTION("Initial state is STATE_UNKNOWN") {
    // After reset, internal_state starts at 0 which is STATE_UNKNOWN
    // We can only verify indirectly by attempting transitions
    // STATE_UNKNOWN can only transition to STATE_STARTED
    int result = state_manager_set_state(STATE_STARTED);
    REQUIRE(result == 0);
  }

  SECTION("Valid full lifecycle transitions") {
    REQUIRE(state_manager_set_state(STATE_STARTED) == 0);
    REQUIRE(state_manager_set_state(STATE_INITIALIZED) == 0);
    REQUIRE(state_manager_set_state(STATE_REGISTERED) == 0);
    REQUIRE(state_manager_set_state(STATE_TIME_SYNCED) == 0);
    REQUIRE(state_manager_set_state(STATE_TEMPO_SYNCED) == 0);
  }

  SECTION("Enter/exit handlers are called on transitions") {
    state_manager_set_state(STATE_STARTED);
    REQUIRE(get_enter_count() == 1);
    REQUIRE(get_exit_count() == 0);

    state_manager_set_state(STATE_INITIALIZED);
    REQUIRE(get_enter_count() == 2);
    REQUIRE(get_exit_count() == 1);
  }

  SECTION("Invalid transition from UNKNOWN to REGISTERED is rejected") {
    int result = state_manager_set_state(STATE_REGISTERED);
    REQUIRE(result == 2);
  }

  SECTION("Invalid transition from UNKNOWN to TIME_SYNCED is rejected") {
    int result = state_manager_set_state(STATE_TIME_SYNCED);
    REQUIRE(result == 2);
  }

  SECTION("Backward transition is rejected") {
    state_manager_set_state(STATE_STARTED);
    state_manager_set_state(STATE_INITIALIZED);

    int result = state_manager_set_state(STATE_STARTED);
    REQUIRE(result == 2);
  }

  SECTION("Self-transition for non-TEMPO_SYNCED is rejected") {
    state_manager_set_state(STATE_STARTED);

    int result = state_manager_set_state(STATE_STARTED);
    REQUIRE(result == 1);
  }

  SECTION("Self-transition for TEMPO_SYNCED is allowed (re-sync)") {
    state_manager_set_state(STATE_STARTED);
    state_manager_set_state(STATE_INITIALIZED);
    state_manager_set_state(STATE_REGISTERED);
    state_manager_set_state(STATE_TIME_SYNCED);
    state_manager_set_state(STATE_TEMPO_SYNCED);

    reset_stub_counters();
    int result = state_manager_set_state(STATE_TEMPO_SYNCED);
    REQUIRE(result == 0);
    // Exit old state + enter new state
    REQUIRE(get_exit_count() == 1);
    REQUIRE(get_enter_count() == 1);
  }

  SECTION("TEMPO_SYNCED can transition back to TIME_SYNCED") {
    state_manager_set_state(STATE_STARTED);
    state_manager_set_state(STATE_INITIALIZED);
    state_manager_set_state(STATE_REGISTERED);
    state_manager_set_state(STATE_TIME_SYNCED);
    state_manager_set_state(STATE_TEMPO_SYNCED);

    int result = state_manager_set_state(STATE_TIME_SYNCED);
    REQUIRE(result == 0);
  }

  SECTION("Out-of-bounds state value is rejected") {
    int result = state_manager_set_state((state_manager_state_t)99);
    REQUIRE(result == 2);
  }

  SECTION("schedule_state_transition adds event to queue") {
    bool scheduled = schedule_state_transition(STATE_STARTED);
    REQUIRE(scheduled);

    event_t event;
    event_queue_pop_message_blocking(&event);
    REQUIRE(event.event_type == event_state_transition);

    state_event_t *state_event = (state_event_t *)event.data;
    REQUIRE(state_event->next_state == STATE_STARTED);

    free(event.data);
  }
}
