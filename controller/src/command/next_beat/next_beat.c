#include <inttypes.h>
#include <stdlib.h>

#include "beatled/protocol.h"
#include "config/constants.h"
#include "clock/clock.h"
#include "command/next_beat.h"
#include "command/qos.h"
#include "command/utils.h"
#include "hal/network.h"
#include "hal/registry.h"
#include "process/intercore_queue.h"
#include "state_manager/state_manager.h"

// Track the last-seen sequence number to detect packet loss. Wraparound is
// handled by treating the signed 16-bit difference as the actual increment.
static uint16_t last_next_beat_seq = 0;
static uint32_t last_next_beat_epoch = 0;
static bool have_seen_next_beat = false;
static uint32_t next_beat_gap_total = 0;

uint32_t next_beat_get_gap_total(void) {
  return next_beat_gap_total;
}

int process_next_beat_msg(beatled_message_t *server_msg, size_t data_length) {

  if (!check_size(data_length, sizeof(beatled_message_next_beat_t))) {
    return 1;
  }

  int current_state = state_manager_get_state();
  if (current_state != STATE_TIME_SYNCED && current_state != STATE_TEMPO_SYNCED) {
    return 0;
  }

  beatled_message_next_beat_t *next_beat_msg = (beatled_message_next_beat_t *)server_msg;

  uint64_t next_beat_time_ref =
      server_time_to_local_time(ntohll(next_beat_msg->next_beat_time_ref));
  uint32_t beat_count = ntohl(next_beat_msg->beat_count);
  uint16_t seq = ntohs(next_beat_msg->seq);
  uint32_t epoch = ntohl(next_beat_msg->epoch);

  // Sequence-gap accounting, scoped to one server-boot epoch. (int16_t) cast
  // handles 16-bit wrap correctly. A new epoch means the server restarted and
  // reset its seq counter; re-anchor without rejecting the message or logging
  // the reset as a flood of lost beats (see protocol v5).
  if (have_seen_next_beat && epoch == last_next_beat_epoch) {
    int16_t delta = (int16_t)(seq - last_next_beat_seq);
    if (delta <= 0) {
      // Stale or duplicate (older or same seq) — drop without applying.
#if BEATLED_VERBOSE_LOG
      printf("[CMD] Stale NEXT_BEAT seq=%u (last=%u), dropping\n", seq, last_next_beat_seq);
#endif
      return 0;
    }
    if (delta > 1) {
      uint32_t lost = (uint32_t)delta - 1;
      next_beat_gap_total += lost;
#if BEATLED_VERBOSE_LOG
      printf("[CMD] NEXT_BEAT gap: %" PRIu32 " missed (seq %u -> %u, total=%" PRIu32 ")\n", lost,
             last_next_beat_seq, seq, next_beat_gap_total);
#endif
    }
  }
  last_next_beat_seq = seq;
  last_next_beat_epoch = epoch;
  have_seen_next_beat = true;

#if BEATLED_VERBOSE_LOG
  printf("[CMD] Next beat: seq=%u ref=%llu (in %lld us) beat=%" PRIu32 "\n", seq,
         next_beat_time_ref, (long long)((int64_t)next_beat_time_ref - (int64_t)time_us_64()),
         beat_count);
#endif

  if (state_manager_get_state() != STATE_TEMPO_SYNCED) {
    if (!schedule_state_transition(STATE_TEMPO_SYNCED)) {
      BEATLED_FATAL("Failed to schedule transition to tempo synced state");
      return 1;
    }
  }

  registry_lock_mutex();
  registry.next_beat_time_ref = next_beat_time_ref;
  registry.beat_count = beat_count;
  // Note: tempo_period_us and program_id are no longer carried on NEXT_BEAT
  // (protocol v2). They come from TEMPO_RESPONSE and PROGRAM push.
  registry.update_timestamp = time_us_64();
  registry_unlock_mutex();

  intercore_message_t msg = {.message_type = INTERCORE_FLAG_TIME_REF_UPDATE};

  if (!hal_queue_add_message(intercore_command_queue, &msg)) {
    qos_intercore_drop_bump();
    puts("[ERR] Intercore queue full, skipping notification");
  }

  return 0;
}
