#ifndef COMMAND__CONSTANTS_H
#define COMMAND__CONSTANTS_H

#include <stdbool.h>
#include <stdint.h>

#include "macros.h"

#ifndef PICO_UNIQUE_BOARD_ID_SIZE_BYTES
#define PICO_UNIQUE_BOARD_ID_SIZE_BYTES 8
#endif // PICO_UNIQUE_BOARD_ID_SIZE_BYTES

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  BEATLED_MESSAGE_ERROR = 0,
  BEATLED_MESSAGE_HELLO_REQUEST,
  BEATLED_MESSAGE_HELLO_RESPONSE,
  BEATLED_MESSAGE_TEMPO_REQUEST,
  BEATLED_MESSAGE_TEMPO_RESPONSE,
  BEATLED_MESSAGE_TIME_REQUEST,
  BEATLED_MESSAGE_TIME_RESPONSE,
  BEATLED_MESSAGE_PROGRAM,
  BEATLED_MESSAGE_NEXT_BEAT,
  BEATLED_MESSAGE_BEAT,
  BEATLED_MESSAGE_LAST_VALUE
} beatled_message_type_t;

typedef enum {
  BEATLED_ERROR_UNKNOWN = 0,
  BEATLED_ERROR_UNKNOWN_MESSAGE_TYPE,
  BEATLED_ERROR_NO_DATA
} beatled_error_codes;

typedef struct {
  uint8_t type; // beatled_message_type_t
} __attribute__((__packed__)) beatled_message_t;

// Tempo message. eCommandType = BEATLED_MESSAGE_ERROR
typedef struct {
  beatled_message_t base;
  uint8_t error_code;
} __attribute__((__packed__)) beatled_message_error_t;

// Tempo message. eCommandType = BEATLED_MESSAGE_TEMPO_REQUEST
//
// Protocol v2: dropped the redundant beat_time_ref/tempo_period echo (the
// server never read them) and added the controller's most recent measured
// one-way delay estimate. The server uses it to compensate NEXT_BEAT
// timestamps per-client so heterogeneous Wi-Fi paths still align.
//
// owd_us_estimate = 0 means "no sample yet, no compensation please".
typedef struct {
  beatled_message_t base;
  uint32_t owd_us_estimate;
} __attribute__((__packed__)) beatled_message_tempo_request_t;

// Tempo message. eCommandType = BEATLED_MESSAGE_TEMPO_RESPONSE
typedef struct {
  beatled_message_t base;
  uint64_t beat_time_ref;
  uint32_t tempo_period_us;
  uint16_t program_id;
} __attribute__((__packed__)) beatled_message_tempo_response_t;

// Hello. eCommandType = BEATLED_MESSAGE_HELLO_REQUEST
typedef struct {
  beatled_message_t base;
  uint8_t board_id[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
} __attribute__((__packed__)) beatled_message_hello_request_t;

// Hello. eCommandType = BEATLED_MESSAGE_HELLO_RESPONSE
typedef struct {
  beatled_message_t base;
  uint16_t client_id;
} __attribute__((__packed__)) beatled_message_hello_response_t;

// eCommandType = BEATLED_MESSAGE_TIME_REQUEST
typedef struct {
  beatled_message_t base;
  uint64_t orig_time;
} __attribute__((__packed__)) beatled_message_time_request_t;

// eCommandType = BEATLED_MESSAGE_TIME_RESPONSE
typedef struct {
  beatled_message_t base;
  uint64_t orig_time;
  uint64_t recv_time;
  uint64_t xmit_time;
} __attribute__((__packed__)) beatled_message_time_response_t;

// eCommandType = BEATLED_MESSAGE_PROGRAM
//
// Protocol v2: gains a sequence number so controllers can ignore stale or
// out-of-order pushes. The server emits one on every program change plus a
// low-rate (~1 Hz) refresh so late joiners and packet-loss don't strand
// controllers on a wrong pattern.
typedef struct {
  beatled_message_t base;
  uint16_t program_id;
  uint16_t seq;
} __attribute__((__packed__)) beatled_message_program_t;

// eCommandType = BEATLED_MESSAGE_NEXT_BEAT
//
// Protocol v2: trimmed from 19 B to 15 B by dropping `tempo_period_us` and
// `program_id` (now carried only by TEMPO_RESPONSE and PROGRAM). Adds a
// monotonically-increasing `seq` so controllers can detect loss and reject
// stale broadcasts.
typedef struct {
  beatled_message_t base;
  uint64_t next_beat_time_ref;
  uint32_t beat_count;
  uint16_t seq;
} __attribute__((__packed__)) beatled_message_next_beat_t;

// eCommandType = BEATLED_MESSAGE_BEAT
//
// Same shape as NEXT_BEAT; currently unused by the live system but kept in
// the catalogue for parity with the beat-detector callback structure.
typedef struct {
  beatled_message_t base;
  uint64_t beat_time_ref;
  uint32_t beat_count;
  uint16_t seq;
} __attribute__((__packed__)) beatled_message_beat_t;

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif // COMMAND__CONSTANTS_H