#include <inttypes.h>
#include <string.h>

#include "hal/unique_id.h"

#include "beatled/protocol.h"
#include "beatled_version.h"
#include "command/hello.h"
#include "command/utils.h"
#include "config/constants.h"
#include "config/port_name.h"
#include "hal/blink.h"
#include "hal/network.h"
#include "hal/time.h"
#include "hal/udp.h"
#include "state_manager/state_manager.h"

// Tracks when we last saw a HELLO_RESPONSE so we can log how long
// we've been trying to register without an answer.
static uint64_t last_hello_response_us = 0;

int prepare_hello_request(void *buffer_payload, size_t buf_len) {
  if (buf_len != sizeof(beatled_message_hello_request_t)) {
    printf("[ERR] Hello request size mismatch: got=%zu expected=%zu\n", buf_len,
           sizeof(beatled_message_hello_request_t));
    return 1;
  }

  beatled_message_hello_request_t *msg = (beatled_message_hello_request_t *)buffer_payload;
  msg->base.type = BEATLED_MESSAGE_HELLO_REQUEST;

  // Protocol-version handshake. Single bytes, no byte-order concern. The
  // server rejects the HELLO if version_major doesn't match its own.
  msg->version_major = BEATLED_PROTOCOL_VERSION_MAJOR;
  msg->version_minor = BEATLED_PROTOCOL_VERSION_MINOR;

  get_unique_board_id(msg->board_id);

  // Firmware self-description for /api/devices. All three are stamped at
  // build time — strncpy with one-byte slack guarantees the NUL even if a
  // future port name is exactly LEN characters.
  memset(msg->port_name, 0, sizeof(msg->port_name));
  strncpy(msg->port_name, BEATLED_PORT_NAME, sizeof(msg->port_name) - 1);
  memset(msg->git_sha, 0, sizeof(msg->git_sha));
  strncpy(msg->git_sha, BEATLED_GIT_HASH, sizeof(msg->git_sha) - 1);
  msg->build_time_us = htonll((uint64_t)BEATLED_BUILD_TIME_US);

  return 0;
}

int send_hello_request() {
  state_manager_state_t st = state_manager_get_state();
  if (last_hello_response_us == 0) {
    printf("[NET] Sending hello request (state=%s, no response yet)\n", state_name(st));
  } else {
    uint64_t since_us = time_us_64() - last_hello_response_us;
    printf("[NET] Sending hello request (state=%s, last response %llus ago)\n", state_name(st),
           (unsigned long long)(since_us / 1000000ULL));
  }
  int err = send_udp_request(sizeof(beatled_message_hello_request_t), prepare_hello_request);
  if (err) {
    printf("[ERR] send_hello_request: send_udp_request returned %d\n", err);
  }
  return err;
}

int process_hello_msg(beatled_message_t *server_msg, size_t data_length) {
  if (!check_size(data_length, sizeof(beatled_message_hello_response_t))) {
    return 1;
  }

  beatled_message_hello_response_t *hello_msg = (beatled_message_hello_response_t *)server_msg;
  uint16_t client_id = ntohs(hello_msg->client_id);
  last_hello_response_us = time_us_64();
  printf("[CMD] Registered with client_id=%d\n", client_id);
  blink(MESSAGE_BLINK_SPEED, MESSAGE_HELLO);

  if (state_manager_get_state() >= STATE_REGISTERED) {
    return 0;
  }

  if (!schedule_state_transition(STATE_REGISTERED)) {
    BEATLED_FATAL("Failed to schedule transition to REGISTERED");
    return 1;
  }

  return 0;
}
