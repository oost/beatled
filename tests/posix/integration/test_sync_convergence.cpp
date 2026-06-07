// Sync-convergence integration tests for protocol v2.
//
// Exercises the controller-side filters that protect against Wi-Fi jitter
// and loss: the median-with-outlier-reject time offset filter, the
// outstanding-orig_time check that drops stale TIME_RESPONSEs, and the
// 16-bit NEXT_BEAT / PROGRAM sequence-number tracking that lets us count
// loss and ignore late duplicates without applying them.

#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <cstring>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

#include "beatled/protocol.h"
#include "clock/clock.h"
#include "command/command.h"
#include "command/next_beat.h"
#include "command/time.h"
#include "event/event_queue.h"
#include "hal/network.h"
#include "hal/queue.h"
#include "hal/registry.h"
#include "hal/time.h"
#include "process/intercore_queue.h"
#include "state_manager/state_manager.h"

#ifdef __cplusplus
}
#endif

extern "C" hal_queue_handle_t event_queue_ptr;
extern "C" hal_queue_handle_t intercore_command_queue;

typedef struct state_manager_internal_state {
  state_manager_state_t current_state;
  int64_t last_tempo_sync_time;
} state_manager_internal_state_t;

extern "C" state_manager_internal_state_t internal_state;
extern "C" exit_state_fn exit_current_state;

extern "C" void stub_reset_counters(void);

namespace {

// Inject a synthetic TIME_RESPONSE round-trip and return what the controller
// computed (we drive it directly through process_time_msg). delay parameter
// controls the spread between recv and dest, so the test can author samples
// that the filter should classify as outliers.
//
//   orig=t0, recv=server_now, xmit=server_now+SERVER_PROC, dest=t0+rtt
//
// In Cristian's algorithm the offset is the difference between server and
// local midpoints; both clocks being equal here makes offset 0, while delay
// = rtt - SERVER_PROC. We piggyback a synthetic offset by shifting the
// server-side timestamps.
struct TimeSample {
  uint64_t orig;
  uint64_t recv;
  uint64_t xmit;
  uint64_t dest;
};

TimeSample synthesise(uint64_t orig, int64_t offset_us, uint64_t rtt_us,
                      uint64_t server_proc_us = 50) {
  TimeSample s;
  s.orig = orig;
  s.dest = orig + rtt_us;
  // Place server timestamps at the midpoint so the computed offset matches.
  uint64_t mid = orig + rtt_us / 2;
  s.recv = (uint64_t)((int64_t)mid + offset_us) - server_proc_us / 2;
  s.xmit = (uint64_t)((int64_t)mid + offset_us) + server_proc_us / 2;
  return s;
}

// Inject one sample into process_time_msg, going through the public API.
void inject_time_sample(const TimeSample &s) {
  time_sync_seed_outstanding_for_testing(s.orig);
  beatled_message_time_response_t msg;
  memset(&msg, 0, sizeof(msg));
  msg.base.type = BEATLED_MESSAGE_TIME_RESPONSE;
  msg.orig_time = htonll(s.orig);
  msg.recv_time = htonll(s.recv);
  msg.xmit_time = htonll(s.xmit);
  process_time_msg((beatled_message_t *)&msg, sizeof(msg), s.dest);
}

void reset_state() {
  if (exit_current_state) {
    exit_current_state();
    exit_current_state = nullptr;
  }
  internal_state.current_state = STATE_UNKNOWN;
  internal_state.last_tempo_sync_time = 0;
  set_server_time_offset(0);
  time_sync_reset_for_testing();
  stub_reset_counters();
}

void advance_to_time_synced() {
  reset_state();
  REQUIRE(state_manager_set_state(STATE_STARTED) == 0);
  event_t e;
  while (hal_queue_pop_message(event_queue_ptr, &e)) {
    handle_event(&e);
  }
  REQUIRE(state_manager_get_state() == STATE_INITIALIZED);

  beatled_message_hello_response_t hello;
  memset(&hello, 0, sizeof(hello));
  hello.base.type = BEATLED_MESSAGE_HELLO_RESPONSE;
  hello.client_id = htons(1);
  void *hdata = malloc(sizeof(hello));
  memcpy(hdata, &hello, sizeof(hello));
  event_t he{event_server_message, 0, sizeof(hello), hdata};
  handle_event(&he);
  while (hal_queue_pop_message(event_queue_ptr, &e)) {
    handle_event(&e);
  }
  REQUIRE(state_manager_get_state() == STATE_REGISTERED);

  // Drive at least one good time sample so we move into TIME_SYNCED.
  TimeSample first = synthesise(/*orig=*/1000, /*offset_us=*/0,
                                /*rtt_us=*/100);
  inject_time_sample(first);
  while (hal_queue_pop_message(event_queue_ptr, &e)) {
    handle_event(&e);
  }
  REQUIRE(state_manager_get_state() == STATE_TIME_SYNCED);
}

} // namespace

TEST_CASE("Median offset filter rejects single Wi-Fi outlier",
          "[integration][sync]") {
  advance_to_time_synced();

  // Seven clean samples around 5000us RTT and offset=+1000us, then one
  // outlier at 100ms RTT with a junk offset. The filter should clamp to the
  // median and ignore the spike.
  const int64_t honest_offset = 1000;
  for (int i = 0; i < 7; i++) {
    inject_time_sample(synthesise(2000 + i, honest_offset, 5000));
  }
  // Outlier: huge RTT, wildly different offset.
  inject_time_sample(synthesise(9999, /*offset=*/-50000,
                                /*rtt_us=*/100000));

  // With 7 clean + 1 outlier, the median of 8 lands inside the clean set so
  // the recovered offset must be very close to `honest_offset`. (Cristian's
  // computation has a 1-bit /2 rounding term so we tolerate a small slop.)
  int64_t got = get_server_time_offset();
  REQUIRE(std::abs(got - honest_offset) <= 4);
}

TEST_CASE("Stale TIME_RESPONSE is dropped",
          "[integration][sync]") {
  advance_to_time_synced();

  // Drive a known-good sample so the offset is non-zero.
  inject_time_sample(synthesise(/*orig=*/3000, /*offset=*/+1234,
                                /*rtt=*/5000));
  int64_t baseline = get_server_time_offset();
  REQUIRE(baseline != 0);

  // Now hand-craft a TIME_RESPONSE that echoes a *different* orig_time
  // than the one most-recently sent. The filter must drop it without
  // touching the offset.
  time_sync_seed_outstanding_for_testing(99999);
  beatled_message_time_response_t resp;
  memset(&resp, 0, sizeof(resp));
  resp.base.type = BEATLED_MESSAGE_TIME_RESPONSE;
  resp.orig_time = htonll(123456);
  resp.recv_time = htonll(123456 + 10);
  resp.xmit_time = htonll(123456 + 20);
  process_time_msg((beatled_message_t *)&resp, sizeof(resp), 123456 + 30);

  REQUIRE(get_server_time_offset() == baseline);
}

TEST_CASE("NEXT_BEAT sequence gaps are counted, stale duplicates ignored",
          "[integration][sync]") {
  advance_to_time_synced();
  uint32_t baseline_gap = next_beat_get_gap_total();

  auto make_next_beat = [](uint16_t seq, uint32_t beat_count) {
    beatled_message_next_beat_t nb;
    memset(&nb, 0, sizeof(nb));
    nb.base.type = BEATLED_MESSAGE_NEXT_BEAT;
    nb.next_beat_time_ref = htonll(1000000ULL + seq);
    nb.beat_count = htonl(beat_count);
    nb.seq = htons(seq);
    return nb;
  };

  // seq 0, 1, 2 in order — no gap.
  for (uint16_t s = 0; s < 3; s++) {
    auto nb = make_next_beat(s, s);
    REQUIRE(process_next_beat_msg((beatled_message_t *)&nb, sizeof(nb)) == 0);
  }
  REQUIRE(next_beat_get_gap_total() == baseline_gap);

  // seq 6 — we skipped 3, 4, 5 → 3 missed beats counted.
  {
    auto nb = make_next_beat(6, 6);
    REQUIRE(process_next_beat_msg((beatled_message_t *)&nb, sizeof(nb)) == 0);
  }
  REQUIRE(next_beat_get_gap_total() == baseline_gap + 3);

  // Replay seq 4 — older than 6, must be dropped without changing the count
  // or the registry's beat_count.
  registry_lock_mutex();
  uint32_t bc_before = registry.beat_count;
  registry_unlock_mutex();
  {
    auto nb = make_next_beat(4, 999);
    REQUIRE(process_next_beat_msg((beatled_message_t *)&nb, sizeof(nb)) == 0);
  }
  REQUIRE(next_beat_get_gap_total() == baseline_gap + 3);
  registry_lock_mutex();
  uint32_t bc_after = registry.beat_count;
  registry_unlock_mutex();
  REQUIRE(bc_after == bc_before);
}

TEST_CASE("PROGRAM with older seq is ignored",
          "[integration][sync]") {
  advance_to_time_synced();

  auto make_program = [](uint16_t program_id, uint16_t seq) {
    beatled_message_program_t pg;
    memset(&pg, 0, sizeof(pg));
    pg.base.type = BEATLED_MESSAGE_PROGRAM;
    pg.program_id = htons(program_id);
    pg.seq = htons(seq);
    return pg;
  };

  // Establish program_id=7 at seq=5.
  {
    auto pg = make_program(7, 5);
    void *data = malloc(sizeof(pg));
    memcpy(data, &pg, sizeof(pg));
    event_t e{event_server_message, 0, sizeof(pg), data};
    REQUIRE(handle_event(&e) == 0);
  }
  registry_lock_mutex();
  REQUIRE(registry.program_id == 7);
  registry_unlock_mutex();

  // Late duplicate (lower seq) carrying a different id must NOT clobber.
  {
    auto pg = make_program(99, 3);
    void *data = malloc(sizeof(pg));
    memcpy(data, &pg, sizeof(pg));
    event_t e{event_server_message, 0, sizeof(pg), data};
    REQUIRE(handle_event(&e) == 0);
  }
  registry_lock_mutex();
  REQUIRE(registry.program_id == 7);
  registry_unlock_mutex();

  // Newer seq does apply.
  {
    auto pg = make_program(42, 9);
    void *data = malloc(sizeof(pg));
    memcpy(data, &pg, sizeof(pg));
    event_t e{event_server_message, 0, sizeof(pg), data};
    REQUIRE(handle_event(&e) == 0);
  }
  registry_lock_mutex();
  REQUIRE(registry.program_id == 42);
  registry_unlock_mutex();
}
