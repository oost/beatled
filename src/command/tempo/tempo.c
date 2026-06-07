#include <inttypes.h>
#include "command/tempo.h"
#include "command/time.h"
#include "beatled/protocol.h"
#include "config/constants.h"
#include "clock/clock.h"
#include "command/utils.h"
#include "hal/network.h"
#include "hal/registry.h"
#include "hal/udp.h"
#include "process/intercore_queue.h"
#include "state_manager/state_manager.h"

int prepare_tempo_request(void *buffer_payload, size_t buf_len) {
  if (buf_len != sizeof(beatled_message_tempo_request_t)) {
    printf("[ERR] Tempo request size mismatch: %zu vs %zu\n", buf_len,
           sizeof(beatled_message_tempo_request_t));
    return 1;
  }

  beatled_message_tempo_request_t *msg = buffer_payload;
  msg->base.type = BEATLED_MESSAGE_TEMPO_REQUEST;
  // Report our current OWD estimate so the server can compensate broadcast
  // timestamps per-client. 0 = "no sample yet, don't compensate".
  msg->owd_us_estimate = htonl(time_sync_owd_estimate_us());
  return 0;
}

int send_tempo_request() {
  return send_udp_request(sizeof(beatled_message_tempo_request_t),
                          prepare_tempo_request);
}

int process_tempo_msg(beatled_message_t *server_msg, size_t data_length) {
  if (!check_size(data_length, sizeof(beatled_message_tempo_response_t))) {
    return 1;
  }

  int current_state = state_manager_get_state();
  if (current_state != STATE_TIME_SYNCED &&
      current_state != STATE_TEMPO_SYNCED) {
    // printf("Can't set tempo while in state %d\n", state_manager_get_state());
    return 1;
  }

  beatled_message_tempo_response_t *tempo_msg =
      (beatled_message_tempo_response_t *)server_msg;

  uint64_t beat_time_ref = ntohll(tempo_msg->beat_time_ref);
  uint32_t tempo_period_us = ntohl(tempo_msg->tempo_period_us);
  uint16_t program_id = ntohs(tempo_msg->program_id);

  uint64_t beat_local_time_ref = server_time_to_local_time(beat_time_ref);
#if BEATLED_VERBOSE_LOG
  printf("[CMD] Tempo update: ref=%llu, period=%"PRIu32" us (%.1f BPM)\n",
         beat_time_ref, tempo_period_us, 1000000.0 * 60 / tempo_period_us);
#endif

  if (current_state != STATE_TEMPO_SYNCED) {
    if (!schedule_state_transition(STATE_TEMPO_SYNCED)) {
      BEATLED_FATAL("Failed to schedule transition to tempo synced state");
    }
  }

  registry_lock_mutex();
  registry.tempo_period_us = tempo_period_us;
  registry.program_id = program_id;
  registry.update_timestamp = time_us_64();
  registry_unlock_mutex();

  // registry_update_t registry_update = {.tempo_time_ref = beat_local_time_ref,
  //                                      .tempo_period_us = tempo_period_us,
  //                                      .program_id = program_id,
  //                                      .update_timestamp = time_us_64(),
  //                                      .registry_update_fields =
  //                                          (0x01 << REGISTRY_UPDATE_TEMPO)};

  intercore_message_t msg = {.message_type = 0x01 << intercore_tempo_update |
                                             0x01 << intercore_program_update};

  if (!hal_queue_add_message(intercore_command_queue, &msg)) {
    puts("[ERR] Intercore queue full, skipping notification");
  }

  return 0;
}