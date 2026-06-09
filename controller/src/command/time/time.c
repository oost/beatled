#include <inttypes.h>

#include "time.h"
#include "beatled/protocol.h"
#include "clock/clock.h"
#include "command/utils.h"
#include "config/constants.h"
#include "hal/network.h"
#include "hal/udp.h"
#include "state_manager/state_manager.h"
#include "state_manager/states.h"

// --- Sliding-window offset filter ------------------------------------------
//
// Each TIME_RESPONSE round-trip gives us one (delay, offset) sample. A single
// sample on Wi-Fi can be tens of ms off because of retransmits or queueing;
// applying it directly leaves the LED phase jittered for the whole refresh
// interval. Instead we maintain a small ring of recent samples, reject the
// ones whose measured delay is more than 2x the median (a Wi-Fi retransmit
// outlier), and apply the median offset of what remains.
//
// The window is small (TIME_SYNC_SAMPLES) because the refresh interval is now
// short (5s) so a fresh-enough median takes ~30-40s to converge after boot.

#define TIME_SYNC_SAMPLES 8

typedef struct {
  uint64_t delay_us;
  int64_t offset_us;
  bool valid;
} time_sample_t;

static time_sample_t samples[TIME_SYNC_SAMPLES];
static size_t sample_write_idx = 0;
static size_t valid_sample_count = 0;

// Cumulative TIME_RESPONSE samples discarded as outliers by the median
// filter (delay > 2 * median(delay)). Saturates at UINT32_MAX; QoS
// snapshot ships this as a single uint32_t.
static uint32_t time_sync_outlier_total_ = 0;

// Most-recently-sent orig_time. The response must echo this exact value or
// we treat it as a stale duplicate from a prior request and drop it.
static uint64_t outstanding_orig_time = 0;
static bool have_outstanding_request = false;

static int compare_uint64(const void *a, const void *b) {
  uint64_t x = *(const uint64_t *)a;
  uint64_t y = *(const uint64_t *)b;
  return (x > y) - (x < y);
}

static int compare_int64(const void *a, const void *b) {
  int64_t x = *(const int64_t *)a;
  int64_t y = *(const int64_t *)b;
  return (x > y) - (x < y);
}

static uint64_t median_delay(void) {
  uint64_t delays[TIME_SYNC_SAMPLES];
  size_t n = 0;
  for (size_t i = 0; i < TIME_SYNC_SAMPLES; i++) {
    if (samples[i].valid) {
      delays[n++] = samples[i].delay_us;
    }
  }
  if (n == 0) {
    return 0;
  }
  qsort(delays, n, sizeof(uint64_t), compare_uint64);
  return delays[n / 2];
}

void time_sync_reset_for_testing(void) {
  for (size_t i = 0; i < TIME_SYNC_SAMPLES; i++) {
    samples[i].valid = false;
  }
  sample_write_idx = 0;
  valid_sample_count = 0;
  outstanding_orig_time = 0;
  have_outstanding_request = false;
  time_sync_outlier_total_ = 0;
}

uint32_t time_sync_median_rtt_us(void) {
  uint64_t med = median_delay();
  return med > UINT32_MAX ? UINT32_MAX : (uint32_t)med;
}

uint32_t time_sync_outlier_total(void) {
  return time_sync_outlier_total_;
}

uint32_t time_sync_valid_sample_count(void) {
  return (uint32_t)valid_sample_count;
}

void time_sync_seed_outstanding_for_testing(uint64_t orig_time) {
  outstanding_orig_time = orig_time;
  have_outstanding_request = true;
}

uint32_t time_sync_owd_estimate_us(void) {
  uint64_t med = median_delay();
  // RTT/2 — we don't have one-way information so this is the best we can do.
  // Cap at uint32 max to keep the wire field encoding sane.
  uint64_t owd = med / 2;
  return owd > UINT32_MAX ? UINT32_MAX : (uint32_t)owd;
}

static int64_t median_offset_excluding_outliers(uint64_t delay_threshold) {
  int64_t offsets[TIME_SYNC_SAMPLES];
  size_t n = 0;
  size_t rejected = 0;
  for (size_t i = 0; i < TIME_SYNC_SAMPLES; i++) {
    if (!samples[i].valid) {
      continue;
    }
    if (samples[i].delay_us <= delay_threshold) {
      offsets[n++] = samples[i].offset_us;
    } else {
      rejected++;
    }
  }
  // Saturating bump — the wire field is uint32_t so wrapping at UINT32_MAX
  // is preferable to a UB increment past the max.
  if (rejected > 0 && time_sync_outlier_total_ + rejected >= time_sync_outlier_total_) {
    time_sync_outlier_total_ += (uint32_t)rejected;
  } else {
    time_sync_outlier_total_ = UINT32_MAX;
  }
  if (n == 0) {
    return 0;
  }
  qsort(offsets, n, sizeof(int64_t), compare_int64);
  return offsets[n / 2];
}

int prepare_time_request(void *buffer_payload, size_t buf_len) {
  if (buf_len != sizeof(beatled_message_time_request_t)) {
    printf("[ERR] Time request size mismatch: %zu vs %zu\n", buf_len,
           sizeof(beatled_message_time_request_t));
    return 1;
  }

  beatled_message_time_request_t *msg = buffer_payload;
  msg->base.type = BEATLED_MESSAGE_TIME_REQUEST;
  uint64_t orig_time = time_us_64();
  msg->orig_time = htonll(orig_time);

  // Remember which orig_time we just sent so we can reject stale responses.
  outstanding_orig_time = orig_time;
  have_outstanding_request = true;

#if BEATLED_VERBOSE_LOG
  printf("[CMD] Sending time request, orig_time=%llu\n", orig_time);
#endif

  return 0;
}

int send_time_request() {
  return send_udp_request(sizeof(beatled_message_time_request_t), &prepare_time_request);
}

int validate_time_msg(beatled_message_t *server_msg, size_t data_length, uint64_t dest_time) {
  if (!check_size(data_length, sizeof(beatled_message_time_response_t))) {
    return 1;
  }
  return 0;
}

int process_time_msg(beatled_message_t *server_msg, size_t data_length, uint64_t dest_time) {
  if (!check_size(data_length, sizeof(beatled_message_time_response_t))) {
    return 1;
  }
  beatled_message_time_response_t *time_resp_msg = (beatled_message_time_response_t *)server_msg;

  uint64_t orig_time = ntohll(time_resp_msg->orig_time);
  uint64_t recv_time = ntohll(time_resp_msg->recv_time);
  uint64_t xmit_time = ntohll(time_resp_msg->xmit_time);

  // A2 prerequisite (A3): drop stale duplicate responses. If the echoed
  // orig_time doesn't match the most-recently-sent one, this is a delayed
  // response from a prior request — applying it would corrupt the offset.
  if (!have_outstanding_request || orig_time != outstanding_orig_time) {
    printf("[CMD] Stale TIME_RESPONSE orig=%llu (expected %llu), dropping\n",
           (unsigned long long)orig_time, (unsigned long long)outstanding_orig_time);
    return 0;
  }
  have_outstanding_request = false;

  uint64_t delay = (dest_time - orig_time) - (xmit_time - recv_time);
  int64_t clock_offset = ((int64_t)(recv_time / 2) - (int64_t)(orig_time / 2)) +
                         ((int64_t)(xmit_time / 2) - (int64_t)(dest_time / 2));

  // Append to the ring.
  samples[sample_write_idx].delay_us = delay;
  samples[sample_write_idx].offset_us = clock_offset;
  samples[sample_write_idx].valid = true;
  sample_write_idx = (sample_write_idx + 1) % TIME_SYNC_SAMPLES;
  if (valid_sample_count < TIME_SYNC_SAMPLES) {
    valid_sample_count++;
  }

  uint64_t med_delay = median_delay();
  // Reject anything > 2x the median delay as a likely Wi-Fi retransmit.
  uint64_t threshold = med_delay * 2 > med_delay ? med_delay * 2 : UINT64_MAX;
  int64_t med_offset = median_offset_excluding_outliers(threshold);

  printf("[CMD] Time sync: delay=%lluus offset=%lldus "
         "(med_delay=%lluus med_offset=%lldus n=%zu)\n",
         (unsigned long long)delay, (long long)clock_offset, (unsigned long long)med_delay,
         (long long)med_offset, valid_sample_count);

  set_server_time_offset(med_offset);

  if (state_manager_get_state() < STATE_TIME_SYNCED) {
    if (!schedule_state_transition(STATE_TIME_SYNCED)) {
      BEATLED_FATAL("Failed to schedule transition to time synced state");
      return 1;
    }
  }

  return 0;
}