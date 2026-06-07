#ifndef COMMAND__TIME__TIME_H
#define COMMAND__TIME__TIME_H

#include <stddef.h>
#include <stdint.h>

#include "beatled/protocol.h"

int send_time_request();

int validate_time_msg(beatled_message_t *server_msg, size_t data_length, uint64_t dest_time);

int process_time_msg(beatled_message_t *server_msg, size_t data_length, uint64_t dest_time);

// Current estimate of one-way delay (server↔controller) in microseconds,
// computed as median(RTT)/2 across the sliding sample ring. Returns 0
// before the ring has any valid samples. Used by TEMPO_REQUEST to report
// OWD back to the server for per-client compensation.
uint32_t time_sync_owd_estimate_us(void);

// Median round-trip-time across the same sliding sample ring (i.e. the
// "delay" half before it's divided to estimate OWD). 0 before any samples.
uint32_t time_sync_median_rtt_us(void);

// Number of TIME_RESPONSE samples discarded as outliers by the median
// filter since boot (delay > 2 * median(delay)). Cumulative; saturates
// at UINT32_MAX.
uint32_t time_sync_outlier_total(void);

// Current fill of the sliding sample ring (0..TIME_SYNC_SAMPLES). Useful
// to know whether the filter has converged yet.
uint32_t time_sync_valid_sample_count(void);

// Reset the median offset filter and clear any tracked outstanding request.
// Intended for tests and (re-)init paths; not exercised on the live device.
void time_sync_reset_for_testing(void);

// Seed the expected `orig_time` echo. Tests use this to drive
// process_time_msg without going through the real prepare/send path.
void time_sync_seed_outstanding_for_testing(uint64_t orig_time);

#endif // COMMAND__TIME__TIME_H