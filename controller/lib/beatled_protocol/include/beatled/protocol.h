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

// Wire-protocol version, carried on every HELLO_REQUEST. Bump MAJOR on any
// incompatible change (a field added/removed/reordered, or a semantic
// change a peer must understand); bump MINOR for backward-compatible
// additions. The server refuses to register a controller whose MAJOR
// differs from its own — see process_hello_request. Both binaries compile
// against this one header, so the constant is the single source of truth.
//
// Invariant: the first two bytes after `base.type` of a HELLO_REQUEST are
// reserved forever as {version_major, version_minor}. A server of any
// major version can therefore always read the peer's major version, even
// when the rest of the message layout has changed.
#define BEATLED_PROTOCOL_VERSION_MAJOR 4
#define BEATLED_PROTOCOL_VERSION_MINOR 0

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
  // Protocol v4: server-initiated diagnostic probe + response.
  BEATLED_MESSAGE_STATUS_REQUEST,
  BEATLED_MESSAGE_STATUS_RESPONSE,
  BEATLED_MESSAGE_LAST_VALUE
} beatled_message_type_t;

typedef enum {
  BEATLED_ERROR_UNKNOWN = 0,
  BEATLED_ERROR_UNKNOWN_MESSAGE_TYPE,
  BEATLED_ERROR_NO_DATA,
  // Returned to a HELLO_REQUEST whose protocol major version doesn't
  // match the server's. The controller is NOT registered; it must be
  // reflashed with firmware built against the same protocol header.
  BEATLED_ERROR_VERSION_MISMATCH
} beatled_error_codes;

typedef struct {
  uint8_t type; // beatled_message_type_t
} __attribute__((__packed__)) beatled_message_t;

// Tempo message. eCommandType = BEATLED_MESSAGE_ERROR
typedef struct {
  beatled_message_t base;
  uint8_t error_code;
} __attribute__((__packed__)) beatled_message_error_t;

// Diagnostic / QoS snapshot block carried piggy-back on TEMPO_REQUEST and
// in STATUS_RESPONSE. The controller fills it in one place via
// `qos_block_fill()` (see controller/src/command/diagnostics/qos.c) so
// the wire layout is computed once. All multi-byte fields are network
// byte order; the controller htonl/htonll-encodes them at fill time.
//
// Sized to 32 B so the extended TEMPO_REQUEST stays well under any
// realistic UDP MTU on Wi-Fi.
typedef struct {
  int64_t current_offset_us;        // controller's view of server-time-offset
  uint64_t uptime_us;               // time_us_64() since boot
  uint32_t median_rtt_us;           // sliding-window median delay (RTT)
  uint32_t next_beat_gap_total;     // cumulative NEXT_BEAT seq gaps observed
  uint32_t intercore_drop_total;    // cumulative intercore-queue drops
  uint32_t time_sync_outlier_total; // cumulative TIME samples rejected by filter
  uint16_t valid_sample_count;      // current depth of the time-sync ring
  uint16_t last_applied_program_seq;
} __attribute__((__packed__)) beatled_qos_block_t;

// Tempo message. eCommandType = BEATLED_MESSAGE_TEMPO_REQUEST
//
// Protocol v4: trails the existing owd_us_estimate with the full
// `beatled_qos_block_t` so every 10 s heartbeat doubles as a passive
// metrics report. The server stores the latest snapshot on
// ClientStatus and surfaces it via /api/devices.
typedef struct {
  beatled_message_t base;
  uint32_t owd_us_estimate;
  beatled_qos_block_t qos;
} __attribute__((__packed__)) beatled_message_tempo_request_t;

// Tempo message. eCommandType = BEATLED_MESSAGE_TEMPO_RESPONSE
typedef struct {
  beatled_message_t base;
  uint64_t beat_time_ref;
  uint32_t tempo_period_us;
  uint16_t program_id;
} __attribute__((__packed__)) beatled_message_tempo_response_t;

// Sizes of the firmware-version fields carried on HELLO_REQUEST. Both
// strings are null-terminated; the constants are exposed so the
// server can mirror them in ClientStatus and the React UI's view of
// devices.
#define BEATLED_PORT_NAME_LEN 16
#define BEATLED_GIT_HASH_LEN 16

// Hello. eCommandType = BEATLED_MESSAGE_HELLO_REQUEST
//
// `version_major` / `version_minor` lead the payload (immediately after
// the type byte) and are the protocol-version handshake: the server
// rejects a HELLO whose `version_major` differs from its own, returning
// BEATLED_ERROR_VERSION_MISMATCH and declining to register the device.
//
// Protocol v3: gained `port_name`, `git_sha`, and `build_time_us` so the
// server's `/api/devices` response can show what each controller is
// running. All three are stamped at firmware build time —
// controller/cmake/gen_version.cmake produces the SHA + build_time and
// controller/src/config/include/config/port_name.h picks the port
// string via the same PICO_PORT / FREERTOS_PORT / ESP32_PORT / POSIX_PORT
// defines used elsewhere.
typedef struct {
  beatled_message_t base;
  uint8_t version_major;
  uint8_t version_minor;
  uint8_t board_id[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
  char port_name[BEATLED_PORT_NAME_LEN];
  char git_sha[BEATLED_GIT_HASH_LEN];
  uint64_t build_time_us;
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
//
// `beat_count` is the count of the beat that fires AT `next_beat_time_ref`
// (not of the beat during which the message was sent). Controllers fold it
// into their local beat grid so patterns keyed on the count stay phase-
// continuous across re-anchors.
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

// eCommandType = BEATLED_MESSAGE_STATUS_REQUEST
//
// Protocol v4: server-initiated probe. The controller echoes
// `server_send_time_us` verbatim on the STATUS_RESPONSE so the server
// can compute a fresh RTT independent of the controller's own time
// sync.
typedef struct {
  beatled_message_t base;
  uint64_t server_send_time_us;
} __attribute__((__packed__)) beatled_message_status_request_t;

// eCommandType = BEATLED_MESSAGE_STATUS_RESPONSE
//
// Protocol v4: controller's reply to a STATUS_REQUEST. Echoes the
// server's send-time for RTT calculation and carries the same
// diagnostic snapshot as the trailing block on TEMPO_REQUEST.
typedef struct {
  beatled_message_t base;
  uint64_t echo_server_send_time_us;
  beatled_qos_block_t qos;
} __attribute__((__packed__)) beatled_message_status_response_t;

// ---------------------------------------------------------------------------
// LED program table — single source of truth for program ids and display
// names. The firmware expands it into its pattern function table
// (controller/src/ws2812/ws2812_patterns.c) and the server expands it into
// the /api/program response (server/src/server/http/api_handler.cpp), so a
// pattern added here appears in both or fails to compile.
//
// X(id, fn_suffix, display_name): `fn_suffix` names the firmware handler
// `pattern_<fn_suffix>`. Ids must stay dense and in table order — the
// firmware indexes its table by program id.
#define BEATLED_PROGRAM_TABLE(X)                                                                   \
  X(0, snakes, "Snakes!")                                                                          \
  X(1, random, "Random data")                                                                      \
  X(2, sparkle, "Sparkles")                                                                        \
  X(3, greys, "Greys")                                                                             \
  X(4, drops, "Drops")                                                                             \
  X(5, solid, "Solid!")                                                                            \
  X(6, fade_grey, "Fade")                                                                          \
  X(7, fade_color, "Fade Colors")                                                                  \
  X(8, off, "Off")

#define BEATLED_PROGRAM_COUNT 9

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif // COMMAND__CONSTANTS_H