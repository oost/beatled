#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

#include "beatled/protocol.h"
#include "clock/clock.h"
#include "command/command.h"
#include "command/time.h"
#include "event/event_queue.h"
#include "hal/network.h"
#include "hal/queue.h"
#include "hal/registry.h"
#include "hal/time.h"
#include "process/intercore_queue.h"
#include "state_manager/state_manager.h"

// Internal symbols for test setup / verification
extern hal_queue_handle_t event_queue_ptr;
extern hal_queue_handle_t intercore_command_queue;

typedef struct state_manager_internal_state {
  state_manager_state_t current_state;
  int64_t last_tempo_sync_time;
} state_manager_internal_state_t;

extern state_manager_internal_state_t internal_state;
extern exit_state_fn exit_current_state;

// Stub call counters (defined in integration_stubs.c)
extern int stub_send_udp_count;
extern int stub_timer_create_count;
extern int stub_timer_cancel_count;
void stub_reset_counters(void);

#ifdef __cplusplus
}
#endif

// ── Helpers ──────────────────────────────────────────────────────────

// Build an event_t wrapping a heap-copied protocol message.
// handle_event() will free the data pointer.
static event_t make_server_event(const void *msg, size_t size,
                                 uint64_t dest_time = 0) {
  void *data = malloc(size);
  REQUIRE(data != nullptr);
  memcpy(data, msg, size);
  event_t event;
  event.event_type = event_server_message;
  event.time = dest_time ? dest_time : time_us_64();
  event.data_length = size;
  event.data = data;
  return event;
}

// Drain and process all pending events from the event queue.
static void process_pending_events() {
  event_t event;
  while (hal_queue_pop_message(event_queue_ptr, &event)) {
    handle_event(&event);
  }
}

// Reset the state machine to UNKNOWN and clear all state.
static void reset_machine() {
  if (exit_current_state) {
    exit_current_state();
    exit_current_state = NULL;
  }
  internal_state.current_state = STATE_UNKNOWN;
  internal_state.last_tempo_sync_time = 0;
  set_server_time_offset(0);
  time_sync_reset_for_testing();
  stub_reset_counters();
}

// Transition to STARTED — the real enter_started_state() initialises
// registry, event queue, and intercore queue, then schedules a
// transition to INITIALIZED.
static void init_system() {
  reset_machine();
  REQUIRE(state_manager_set_state(STATE_STARTED) == 0);
}

// Default NTP timestamps for time sync helper.
static const uint64_t DEFAULT_ORIG = 10000;
static const uint64_t DEFAULT_RECV = 20000;
static const uint64_t DEFAULT_XMIT = 20100;
static const uint64_t DEFAULT_DEST = 10200;

// Advance the system to the target state by injecting mock server
// messages and processing the resulting state transitions.  Uses the
// real enter/exit handlers at every step.
static void advance_to(state_manager_state_t target) {
  if (state_manager_get_state() < STATE_INITIALIZED && target >= STATE_INITIALIZED) {
    // enter_started_state scheduled INITIALIZED — process it.
    process_pending_events();
    REQUIRE(state_manager_get_state() == STATE_INITIALIZED);
  }

  if (state_manager_get_state() < STATE_REGISTERED && target >= STATE_REGISTERED) {
    // Inject HELLO_RESPONSE to trigger transition to REGISTERED.
    beatled_message_hello_response_t hello;
    memset(&hello, 0, sizeof(hello));
    hello.base.type = BEATLED_MESSAGE_HELLO_RESPONSE;
    hello.client_id = htons(1);
    event_t e = make_server_event(&hello, sizeof(hello));
    handle_event(&e);
    process_pending_events();
    REQUIRE(state_manager_get_state() == STATE_REGISTERED);
  }

  if (state_manager_get_state() < STATE_TIME_SYNCED && target >= STATE_TIME_SYNCED) {
    // Inject TIME_RESPONSE to trigger transition to TIME_SYNCED. v2
    // requires us to pre-seed the expected orig_time so the response is
    // accepted as the reply to a (notionally-)outstanding request.
    time_sync_seed_outstanding_for_testing(DEFAULT_ORIG);
    beatled_message_time_response_t time_msg;
    memset(&time_msg, 0, sizeof(time_msg));
    time_msg.base.type = BEATLED_MESSAGE_TIME_RESPONSE;
    time_msg.orig_time = htonll(DEFAULT_ORIG);
    time_msg.recv_time = htonll(DEFAULT_RECV);
    time_msg.xmit_time = htonll(DEFAULT_XMIT);
    event_t e = make_server_event(&time_msg, sizeof(time_msg), DEFAULT_DEST);
    handle_event(&e);
    process_pending_events();
    REQUIRE(state_manager_get_state() == STATE_TIME_SYNCED);
  }

  if (state_manager_get_state() < STATE_TEMPO_SYNCED && target >= STATE_TEMPO_SYNCED) {
    // Inject TEMPO_RESPONSE to trigger transition to TEMPO_SYNCED.
    beatled_message_tempo_response_t tempo;
    memset(&tempo, 0, sizeof(tempo));
    tempo.base.type = BEATLED_MESSAGE_TEMPO_RESPONSE;
    tempo.beat_time_ref = htonll(1000000);
    tempo.tempo_period_us = htonl(500000);
    tempo.program_id = htons(1);
    event_t e = make_server_event(&tempo, sizeof(tempo));
    handle_event(&e);
    process_pending_events();
    REQUIRE(state_manager_get_state() == STATE_TEMPO_SYNCED);
  }
}

// Pop one intercore message (asserts it exists).
static intercore_message_t pop_intercore_msg() {
  intercore_message_t msg;
  REQUIRE(hal_queue_pop_message(intercore_command_queue, &msg));
  return msg;
}

// Drain leftover intercore messages.
static void drain_intercore_queue() {
  intercore_message_t msg;
  while (hal_queue_pop_message(intercore_command_queue, &msg)) {
  }
}

// ── Tests ────────────────────────────────────────────────────────────

TEST_CASE("Full lifecycle: UNKNOWN to TEMPO_SYNCED via server messages",
          "[integration]") {
  init_system();

  // ── STARTED → INITIALIZED (auto-scheduled by enter_started_state) ──
  process_pending_events();
  REQUIRE(state_manager_get_state() == STATE_INITIALIZED);
  // enter_initialized_state created the hello timer
  REQUIRE(stub_timer_create_count == 1);

  // ── Inject HELLO_RESPONSE → REGISTERED ──
  beatled_message_hello_response_t hello_msg;
  memset(&hello_msg, 0, sizeof(hello_msg));
  hello_msg.base.type = BEATLED_MESSAGE_HELLO_RESPONSE;
  hello_msg.client_id = htons(7);

  event_t hello_event = make_server_event(&hello_msg, sizeof(hello_msg));
  REQUIRE(handle_event(&hello_event) == 0);

  process_pending_events();
  REQUIRE(state_manager_get_state() == STATE_REGISTERED);
  // exit_initialized_state cancelled the hello timer
  REQUIRE(stub_timer_cancel_count == 1);
  // enter_registered_state sent a time request
  REQUIRE(stub_send_udp_count == 1);

  // ── Inject TIME_RESPONSE → TIME_SYNCED ──
  uint64_t orig = 1000, recv_t = 3000, xmit = 4000, dest = 2000;
  time_sync_seed_outstanding_for_testing(orig);

  beatled_message_time_response_t time_msg;
  memset(&time_msg, 0, sizeof(time_msg));
  time_msg.base.type = BEATLED_MESSAGE_TIME_RESPONSE;
  time_msg.orig_time = htonll(orig);
  time_msg.recv_time = htonll(recv_t);
  time_msg.xmit_time = htonll(xmit);

  event_t time_event = make_server_event(&time_msg, sizeof(time_msg), dest);
  REQUIRE(handle_event(&time_event) == 0);
  REQUIRE(get_server_time_offset() != 0);

  process_pending_events();
  REQUIRE(state_manager_get_state() == STATE_TIME_SYNCED);
  // enter_time_synced_state sent a tempo request
  REQUIRE(stub_send_udp_count == 2);

  // ── Inject TEMPO_RESPONSE → TEMPO_SYNCED ──
  beatled_message_tempo_response_t tempo_msg;
  memset(&tempo_msg, 0, sizeof(tempo_msg));
  tempo_msg.base.type = BEATLED_MESSAGE_TEMPO_RESPONSE;
  tempo_msg.beat_time_ref = htonll(5000000);
  tempo_msg.tempo_period_us = htonl(500000); // 120 BPM
  tempo_msg.program_id = htons(3);

  event_t tempo_event = make_server_event(&tempo_msg, sizeof(tempo_msg));
  REQUIRE(handle_event(&tempo_event) == 0);

  // Registry should be updated before state transition
  REQUIRE(registry.tempo_period_us == 500000);
  REQUIRE(registry.program_id == 3);

  // Intercore message should be queued
  intercore_message_t ic_msg = pop_intercore_msg();
  REQUIRE((ic_msg.message_type & (0x01 << intercore_tempo_update)) != 0);
  REQUIRE((ic_msg.message_type & (0x01 << intercore_program_update)) != 0);

  process_pending_events();
  REQUIRE(state_manager_get_state() == STATE_TEMPO_SYNCED);
  // enter_tempo_synced_state created 3 repeating timers
  REQUIRE(stub_timer_create_count == 4); // 1 hello (INITIALIZED) + 3 (TEMPO_SYNCED)
}

TEST_CASE("State entry handlers perform correct setup", "[integration]") {
  init_system();
  stub_reset_counters();

  SECTION("enter_started_state initialises data structures") {
    // Already called by init_system.  Verify the queues exist.
    REQUIRE(event_queue_ptr != nullptr);
    REQUIRE(intercore_command_queue != nullptr);
    // MAX_INTERCORE_QUEUE_COUNT bumped from 64 → 128 for backpressure headroom.
    REQUIRE(hal_queue_capacity(intercore_command_queue) == 128);
  }

  SECTION("enter_initialized_state starts hello timer") {
    process_pending_events(); // STARTED → INITIALIZED
    REQUIRE(stub_timer_create_count == 1);
  }

  SECTION("enter_registered_state sends time request") {
    advance_to(STATE_REGISTERED);
    // The real enter_registered_state calls send_time_request()
    REQUIRE(stub_send_udp_count >= 1);
  }

  SECTION("enter_time_synced_state sends tempo request") {
    advance_to(STATE_TIME_SYNCED);
    // send_time_request (entering REGISTERED) + send_tempo_request (entering TIME_SYNCED)
    REQUIRE(stub_send_udp_count >= 2);
  }

  SECTION("enter_tempo_synced_state creates 3 repeating timers") {
    advance_to(STATE_TEMPO_SYNCED);
    // 1 hello timer (INITIALIZED) + 3 sync timers (TEMPO_SYNCED)
    REQUIRE(stub_timer_create_count == 4);
  }

  SECTION("tempo re-sync updates registry without state re-entry") {
    advance_to(STATE_TEMPO_SYNCED);
    int cancel_before = stub_timer_cancel_count;

    // Send another tempo while already in TEMPO_SYNCED
    beatled_message_tempo_response_t tempo;
    memset(&tempo, 0, sizeof(tempo));
    tempo.base.type = BEATLED_MESSAGE_TEMPO_RESPONSE;
    tempo.beat_time_ref = htonll(2000000);
    tempo.tempo_period_us = htonl(400000);
    tempo.program_id = htons(2);
    event_t e = make_server_event(&tempo, sizeof(tempo));
    handle_event(&e);
    process_pending_events();

    // No state re-entry, so no timers cancelled
    REQUIRE(stub_timer_cancel_count == cancel_before);
    // Registry is still updated
    REQUIRE(registry.tempo_period_us == 400000);
    REQUIRE(registry.program_id == 2);
  }
}

TEST_CASE("Time sync calculates correct clock offset", "[integration]") {
  init_system();
  advance_to(STATE_REGISTERED);

  // NTP calculation:
  // offset = (recv/2 - orig/2) + (xmit/2 - dest/2)
  // With: orig=10000, recv=20000, xmit=20100, dest=10200
  // offset = (10000 - 5000) + (10050 - 5100) = 5000 + 4950 = 9950
  uint64_t orig = 10000, recv_t = 20000, xmit = 20100, dest = 10200;
  time_sync_seed_outstanding_for_testing(orig);

  beatled_message_time_response_t time_msg;
  memset(&time_msg, 0, sizeof(time_msg));
  time_msg.base.type = BEATLED_MESSAGE_TIME_RESPONSE;
  time_msg.orig_time = htonll(orig);
  time_msg.recv_time = htonll(recv_t);
  time_msg.xmit_time = htonll(xmit);

  event_t event = make_server_event(&time_msg, sizeof(time_msg), dest);
  REQUIRE(handle_event(&event) == 0);

  int64_t expected_offset =
      ((int64_t)(recv_t / 2) - (int64_t)(orig / 2)) +
      ((int64_t)(xmit / 2) - (int64_t)(dest / 2));
  REQUIRE(expected_offset == 9950);
  REQUIRE(get_server_time_offset() == expected_offset);

  // Verify server_time_to_local_time uses the offset correctly
  uint64_t server_time = 50000;
  uint64_t local_time = server_time_to_local_time(server_time);
  // offset > 0 → local = server - offset
  REQUIRE(local_time == server_time - (uint64_t)expected_offset);

  process_pending_events();
  REQUIRE(state_manager_get_state() == STATE_TIME_SYNCED);
}

TEST_CASE("Tempo response updates registry and queues intercore message",
          "[integration]") {
  init_system();
  advance_to(STATE_TIME_SYNCED);
  drain_intercore_queue();

  beatled_message_tempo_response_t tempo_msg;
  memset(&tempo_msg, 0, sizeof(tempo_msg));
  tempo_msg.base.type = BEATLED_MESSAGE_TEMPO_RESPONSE;
  tempo_msg.beat_time_ref = htonll(1000000);
  tempo_msg.tempo_period_us = htonl(600000); // 100 BPM
  tempo_msg.program_id = htons(42);

  event_t event = make_server_event(&tempo_msg, sizeof(tempo_msg));
  REQUIRE(handle_event(&event) == 0);

  REQUIRE(registry.tempo_period_us == 600000);
  REQUIRE(registry.program_id == 42);
  REQUIRE(registry.update_timestamp > 0);

  intercore_message_t ic = pop_intercore_msg();
  REQUIRE((ic.message_type & (0x01 << intercore_tempo_update)) != 0);
  REQUIRE((ic.message_type & (0x01 << intercore_program_update)) != 0);

  process_pending_events();
  REQUIRE(state_manager_get_state() == STATE_TEMPO_SYNCED);
}

TEST_CASE("Next beat updates registry with beat timing", "[integration]") {
  init_system();
  advance_to(STATE_TEMPO_SYNCED);
  drain_intercore_queue();

  beatled_message_next_beat_t nb_msg;
  memset(&nb_msg, 0, sizeof(nb_msg));
  nb_msg.base.type = BEATLED_MESSAGE_NEXT_BEAT;
  nb_msg.next_beat_time_ref = htonll(9000000);
  nb_msg.beat_count = htonl(16);
  nb_msg.seq = htons(1);

  event_t event = make_server_event(&nb_msg, sizeof(nb_msg));
  REQUIRE(handle_event(&event) == 0);

  // Protocol v2: NEXT_BEAT no longer carries tempo_period_us or program_id,
  // so it only updates the time_ref / beat_count fields.
  REQUIRE(registry.beat_count == 16);
  REQUIRE(registry.next_beat_time_ref > 0);
  REQUIRE(registry.update_timestamp > 0);

  intercore_message_t ic = pop_intercore_msg();
  REQUIRE((ic.message_type & (0x01 << intercore_time_ref_update)) != 0);

  REQUIRE(state_manager_get_state() == STATE_TEMPO_SYNCED);
}

TEST_CASE("Tempo and next_beat rejected before TIME_SYNCED",
          "[integration]") {
  init_system();
  advance_to(STATE_REGISTERED);
  drain_intercore_queue();

  SECTION("Tempo response rejected in REGISTERED state") {
    beatled_message_tempo_response_t tempo_msg;
    memset(&tempo_msg, 0, sizeof(tempo_msg));
    tempo_msg.base.type = BEATLED_MESSAGE_TEMPO_RESPONSE;
    tempo_msg.beat_time_ref = htonll(1000000);
    tempo_msg.tempo_period_us = htonl(500000);
    tempo_msg.program_id = htons(1);

    event_t event = make_server_event(&tempo_msg, sizeof(tempo_msg));
    REQUIRE(handle_event(&event) == 1);
    REQUIRE(state_manager_get_state() == STATE_REGISTERED);
  }

  SECTION("Next beat ignored in REGISTERED state") {
    beatled_message_next_beat_t nb_msg;
    memset(&nb_msg, 0, sizeof(nb_msg));
    nb_msg.base.type = BEATLED_MESSAGE_NEXT_BEAT;
    nb_msg.next_beat_time_ref = htonll(9000000);
    nb_msg.beat_count = htonl(1);
    nb_msg.seq = htons(1);

    event_t event = make_server_event(&nb_msg, sizeof(nb_msg));
    REQUIRE(handle_event(&event) == 0);
    REQUIRE(state_manager_get_state() == STATE_REGISTERED);

    intercore_message_t ic;
    REQUIRE_FALSE(hal_queue_pop_message(intercore_command_queue, &ic));
  }
}

TEST_CASE("Program change updates registry and queues intercore message",
          "[integration]") {
  init_system();
  advance_to(STATE_TEMPO_SYNCED);
  drain_intercore_queue();

  beatled_message_program_t prog_msg;
  memset(&prog_msg, 0, sizeof(prog_msg));
  prog_msg.base.type = BEATLED_MESSAGE_PROGRAM;
  prog_msg.program_id = htons(99);
  prog_msg.seq = htons(1);

  event_t event = make_server_event(&prog_msg, sizeof(prog_msg));
  REQUIRE(handle_event(&event) == 0);

  REQUIRE(registry.program_id == 99);

  intercore_message_t ic = pop_intercore_msg();
  REQUIRE((ic.message_type & (0x01 << intercore_program_update)) != 0);
}

TEST_CASE("Error message handled without state change", "[integration]") {
  init_system();
  advance_to(STATE_REGISTERED);

  state_manager_state_t state_before = state_manager_get_state();

  beatled_message_error_t err_msg;
  memset(&err_msg, 0, sizeof(err_msg));
  err_msg.base.type = BEATLED_MESSAGE_ERROR;
  err_msg.error_code = 42;

  event_t event = make_server_event(&err_msg, sizeof(err_msg));
  REQUIRE(handle_event(&event) == 0);
  REQUIRE(state_manager_get_state() == state_before);
}

TEST_CASE("Multiple tempo re-syncs cycle correctly", "[integration]") {
  init_system();
  advance_to(STATE_TEMPO_SYNCED);

  // First re-sync: new tempo
  drain_intercore_queue();
  int cancel_before = stub_timer_cancel_count;
  {
    beatled_message_tempo_response_t tempo_msg;
    memset(&tempo_msg, 0, sizeof(tempo_msg));
    tempo_msg.base.type = BEATLED_MESSAGE_TEMPO_RESPONSE;
    tempo_msg.beat_time_ref = htonll(2000000);
    tempo_msg.tempo_period_us = htonl(400000); // 150 BPM
    tempo_msg.program_id = htons(10);

    event_t event = make_server_event(&tempo_msg, sizeof(tempo_msg));
    REQUIRE(handle_event(&event) == 0);
    process_pending_events();
  }
  REQUIRE(state_manager_get_state() == STATE_TEMPO_SYNCED);
  REQUIRE(registry.tempo_period_us == 400000);
  REQUIRE(registry.program_id == 10);
  // No state re-entry when already in TEMPO_SYNCED, so no timers cancelled
  REQUIRE(stub_timer_cancel_count == cancel_before);

  // Second re-sync: next_beat. Protocol v2 NEXT_BEAT only carries
  // next_beat_time_ref + beat_count + seq.
  drain_intercore_queue();
  {
    beatled_message_next_beat_t nb_msg;
    memset(&nb_msg, 0, sizeof(nb_msg));
    nb_msg.base.type = BEATLED_MESSAGE_NEXT_BEAT;
    nb_msg.next_beat_time_ref = htonll(3000000);
    nb_msg.beat_count = htonl(32);
    nb_msg.seq = htons(2);

    event_t event = make_server_event(&nb_msg, sizeof(nb_msg));
    REQUIRE(handle_event(&event) == 0);
  }
  REQUIRE(state_manager_get_state() == STATE_TEMPO_SYNCED);
  REQUIRE(registry.beat_count == 32);

  intercore_message_t ic = pop_intercore_msg();
  REQUIRE((ic.message_type & (0x01 << intercore_time_ref_update)) != 0);
}

TEST_CASE("Hello response with wrong size is rejected", "[integration]") {
  init_system();
  advance_to(STATE_INITIALIZED);

  beatled_message_hello_response_t hello_msg;
  memset(&hello_msg, 0, sizeof(hello_msg));
  hello_msg.base.type = BEATLED_MESSAGE_HELLO_RESPONSE;
  hello_msg.client_id = htons(1);

  uint8_t oversized[sizeof(hello_msg) + 4];
  memcpy(oversized, &hello_msg, sizeof(hello_msg));

  event_t event = make_server_event(oversized, sizeof(oversized));
  REQUIRE(handle_event(&event) == 1);
  REQUIRE(state_manager_get_state() == STATE_INITIALIZED);
}
