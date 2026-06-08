#include <stdio.h>
#include <stdlib.h>

#include "beatled/protocol.h"
#include "command/command.h"
#include "command/next_beat.h"
#include "command/qos.h"
#include "command/status.h"
#include "command/tempo.h"
#include "command/time.h"
#include "command/utils.h"
#include "config/constants.h"
#include "hal/blink.h"
#include "hal/network.h"
#include "hal/registry.h"
#include "process/intercore_queue.h"
#include "state_manager/state_manager.h"

// Track the last-seen PROGRAM sequence number to drop late duplicates.
// Unlike NEXT_BEAT we don't count gaps — PROGRAM repeats every 1s anyway.
static uint16_t last_program_seq = 0;
static bool have_seen_program = false;

int command_program(beatled_message_t *server_msg, size_t data_length) {
  if (!check_size(data_length, sizeof(beatled_message_program_t))) {
    return 1;
  }
  beatled_message_program_t *program_msg = (beatled_message_program_t *)server_msg;

  uint16_t program_id = ntohs(program_msg->program_id);
  uint16_t seq = ntohs(program_msg->seq);

  if (have_seen_program) {
    int16_t delta = (int16_t)(seq - last_program_seq);
    if (delta < 0) {
      // Older than what we've applied — ignore.
      return 0;
    }
  }
  last_program_seq = seq;
  have_seen_program = true;

  printf("[CMD] Program push: id=%u seq=%u\n", program_id, seq);

  registry_lock_mutex();
  bool changed = registry.program_id != program_id;
  registry.program_id = program_id;
  registry_unlock_mutex();

  if (!changed) {
    return 0;
  }

  intercore_message_t msg = {.message_type = 0x01 << intercore_program_update};
  if (!hal_queue_add_message(intercore_command_queue, &msg)) {
    qos_intercore_drop_bump();
    puts("[ERR] Intercore queue full, skipping notification");
  }

  return 0;
}

uint16_t program_get_last_applied_seq(void) {
  return have_seen_program ? last_program_seq : 0;
}

int command_error(beatled_message_t *server_msg, size_t data_length) {
  if (!check_size(data_length, sizeof(beatled_message_error_t))) {
    return 1;
  }
  beatled_message_error_t *error_msg = (beatled_message_error_t *)server_msg;

  if (error_msg->error_code == BEATLED_ERROR_VERSION_MISMATCH) {
    // The server speaks a different protocol major version. Retrying won't
    // help — this firmware must be rebuilt/reflashed against the server's
    // protocol header. Make that unmistakable in the logs.
    printf("[CMD] Server rejected HELLO: protocol major version mismatch. "
           "This firmware is protocol v%d.%d — reflash to match the server.\n",
           BEATLED_PROTOCOL_VERSION_MAJOR, BEATLED_PROTOCOL_VERSION_MINOR);
    return 0;
  }

  printf("[CMD] Server error: code=%u\n", error_msg->error_code);
  return 0;
}

int validate_server_message(void *event_data, size_t data_length) {
  if (!event_data || sizeof(beatled_message_t) > data_length) {
    return 1;
  }
  beatled_message_t *server_msg = (beatled_message_t *)event_data;

  int err = 0;
  switch (server_msg->type) {
  case BEATLED_MESSAGE_HELLO_RESPONSE:
    err = !(sizeof(beatled_message_hello_response_t) == data_length);
    break;

  case BEATLED_MESSAGE_PROGRAM:
    err = !(sizeof(beatled_message_program_t) == data_length);
    break;

  case BEATLED_MESSAGE_TEMPO_RESPONSE:
    err = !(sizeof(beatled_message_tempo_response_t) == data_length);
    break;

  case BEATLED_MESSAGE_TIME_RESPONSE:
    err = !(sizeof(beatled_message_time_response_t) == data_length);
    break;

  case BEATLED_MESSAGE_NEXT_BEAT:
    err = !(sizeof(beatled_message_next_beat_t) == data_length);
    break;

  case BEATLED_MESSAGE_STATUS_REQUEST:
    err = !(sizeof(beatled_message_status_request_t) == data_length);
    break;

  case BEATLED_MESSAGE_ERROR:
    err = !(sizeof(beatled_message_error_t) == data_length);
    break;

  default:
    printf("[CMD] Unknown command type %d\n", server_msg->type);
    blink(ERROR_BLINK_SPEED, ERROR_COMMAND);
    err = 1;
  }

  return err;
}

int handle_server_message(void *event_data, size_t data_length, uint64_t dest_time) {
  if (!event_data || sizeof(beatled_message_t) > data_length) {
    return 1;
  }
  beatled_message_t *server_msg = (beatled_message_t *)event_data;

  int err = 0;
  switch (server_msg->type) {
  case BEATLED_MESSAGE_HELLO_RESPONSE:
#if BEATLED_VERBOSE_LOG
    printf("[MSG] Received HELLO_RESPONSE\n");
#endif
    err = process_hello_msg(server_msg, data_length);
    break;

  case BEATLED_MESSAGE_PROGRAM:
    err = command_program(server_msg, data_length);
    break;

  case BEATLED_MESSAGE_TEMPO_RESPONSE:
#if BEATLED_VERBOSE_LOG
    printf("[MSG] Received TEMPO_RESPONSE\n");
#endif
    err = process_tempo_msg(server_msg, data_length);
    break;

  case BEATLED_MESSAGE_TIME_RESPONSE:
#if BEATLED_VERBOSE_LOG
    printf("[MSG] Received TIME_RESPONSE\n");
#endif
    err = process_time_msg(server_msg, data_length, dest_time);
    break;

  case BEATLED_MESSAGE_NEXT_BEAT:
    err = process_next_beat_msg(server_msg, data_length);
    break;

  case BEATLED_MESSAGE_STATUS_REQUEST:
    err = process_status_request(server_msg, data_length);
    break;

  case BEATLED_MESSAGE_ERROR:
    err = command_error(server_msg, data_length);
    break;

  default:
    printf("[MSG] Unknown message type %d\n", server_msg->type);
    blink(ERROR_BLINK_SPEED, ERROR_COMMAND);
    err = 1;
  }

  return err;
}

int handle_state_change(void *event_data) {
  state_event_t *state_event = (state_event_t *)event_data;
  return state_manager_set_state(state_event->next_state);
}

int handle_event(event_t *event) {
  // puts("Handling event");
  int err;
  switch (event->event_type) {
  case event_server_message:
    err = validate_server_message(event->data, event->data_length);
    if (err) {
      // Malformed or unsolicited packet — drop without dispatching to the
      // message handlers, which assume exact-size payloads.
      printf("[ERR] Dropping invalid server message (len=%zu)\n", event->data_length);
      break;
    }
    err = handle_server_message(event->data, event->data_length, event->time);
    break;

  case event_state_transition:
    err = handle_state_change(event->data);
    break;

  case event_error:
    puts("[ERR] Error event received");
    blink(ERROR_BLINK_SPEED, ERROR_COMMAND);
    err = 1;
    break;

  default:
    printf("[ERR] Unknown event type %d\n", event->event_type);
    blink(ERROR_BLINK_SPEED, ERROR_COMMAND);
    err = 1;
  }
  // puts("Event handled.");
  if (event->data != NULL) {
    // puts("Deleteing event data.");
    // printf("Freeing memory at %x\n", event->data);
    free(event->data);
  }

  return err;
}
