#include <string.h>

#include "command/command.h"
#include "command/next_beat.h"
#include "command/qos.h"
#include "command/time.h"
#include "beatled/protocol.h"
#include "clock/clock.h"
#include "hal/network.h"
#include "hal/time.h"

// Cumulative count of failed intercore_command_queue add attempts. The
// bump sites are at the producer ends of the queue (NEXT_BEAT, PROGRAM,
// TEMPO handlers in command.c / tempo.c / next_beat.c). Unsigned wrap is
// the desired behaviour at UINT32_MAX.
static uint32_t intercore_drop_total_ = 0;

// Boot time, lazily captured the first time qos_block_fill is asked for
// it. time_us_64() on the Pico SDK monotonic clock starts at 0 at boot,
// so storing the first observation is enough to anchor the uptime
// snapshot we ship over the wire.
static uint64_t boot_time_us_ = 0;

void qos_intercore_drop_bump(void) {
  intercore_drop_total_++;
}

uint32_t qos_intercore_drop_total(void) {
  return intercore_drop_total_;
}

void qos_block_fill(beatled_qos_block_t *out) {
  if (!out) {
    return;
  }
  memset(out, 0, sizeof(*out));

  uint64_t now = time_us_64();
  if (boot_time_us_ == 0) {
    boot_time_us_ = now;
  }
  uint64_t uptime = now - boot_time_us_;

  int64_t offset = get_server_time_offset();
  uint32_t median_rtt_us = time_sync_median_rtt_us();
  uint32_t outliers = time_sync_outlier_total();
  uint32_t valid = time_sync_valid_sample_count();
  uint32_t next_beat_gaps = next_beat_get_gap_total();
  uint32_t intercore_drops = intercore_drop_total_;
  uint16_t last_program_seq = (uint16_t)program_get_last_applied_seq();

  // Encode network byte order — signed offset goes through the same
  // bit pattern via a uint64 round-trip so big- and little-endian
  // controllers stay consistent.
  uint64_t offset_bits;
  memcpy(&offset_bits, &offset, sizeof(offset_bits));
  out->current_offset_us = (int64_t)htonll(offset_bits);
  out->uptime_us = htonll(uptime);
  out->median_rtt_us = htonl(median_rtt_us);
  out->next_beat_gap_total = htonl(next_beat_gaps);
  out->intercore_drop_total = htonl(intercore_drops);
  out->time_sync_outlier_total = htonl(outliers);
  out->valid_sample_count = htons((uint16_t)valid);
  out->last_applied_program_seq = htons(last_program_seq);
}

void qos_reset_for_testing(void) {
  intercore_drop_total_ = 0;
  boot_time_us_ = 0;
}
