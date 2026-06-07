#include <stdio.h>

#include "beatled/network.h"
#include "beatled/protocol.h"
#include "command/qos.h"
#include "command/status.h"
#include "command/utils.h"
#include "hal/udp.h"

// The UDP-send plumbing's prepare_payload callback doesn't carry user
// state, so the STATUS_REQUEST handler stashes the echoed timestamp
// here for the matching response-prep callback. The command-dispatch
// pipeline on the controller is single-threaded, so a static is fine.
static uint64_t pending_echo_time_us_be_ = 0;

static int prepare_status_response(void *buffer_payload, size_t buf_len) {
  if (buf_len != sizeof(beatled_message_status_response_t)) {
    printf("[ERR] Status response size mismatch: %zu vs %zu\n", buf_len,
           sizeof(beatled_message_status_response_t));
    return 1;
  }
  beatled_message_status_response_t *msg = buffer_payload;
  msg->base.type = BEATLED_MESSAGE_STATUS_RESPONSE;
  msg->echo_server_send_time_us = pending_echo_time_us_be_; // already big-endian
  qos_block_fill(&msg->qos);
  return 0;
}

int process_status_request(beatled_message_t *server_msg, size_t data_length) {
  if (!check_size(data_length, sizeof(beatled_message_status_request_t))) {
    return 1;
  }
  beatled_message_status_request_t *req = (beatled_message_status_request_t *)server_msg;
  // Stash the server's send time in network byte order; we echo it back
  // verbatim so the server can compute fresh RTT without coordinating on
  // endianness.
  pending_echo_time_us_be_ = req->server_send_time_us;

  int err = send_udp_request(sizeof(beatled_message_status_response_t), prepare_status_response);
  if (err) {
    printf("[ERR] process_status_request: send_udp_request returned %d\n", err);
  }
  return err;
}
