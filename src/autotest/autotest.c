
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "autotest.h"
#include "beatled/protocol.h"
#include "clock/clock.h"
#include "event/event_queue.h"
#include "hal/network.h"

static uint64_t server_time_ref_us;

void init_test() {
  // server_time_ref_us = get_sntp_server_time_ref_us();
}

void test_tempo() {
  float tempo = 60;

  beatled_message_tempo_response_t *tempo_msg =
      (beatled_message_tempo_response_t *)malloc(
          sizeof(beatled_message_tempo_response_t));
  if (!tempo_msg) {
    puts("[ERR] Failed to allocate tempo message");
    return;
  }

  tempo_msg->base.type = BEATLED_MESSAGE_TEMPO_RESPONSE;

  printf("[TEST] Time since boot: %llu (%llx)\n", server_time_ref_us,
         server_time_ref_us);
  tempo_msg->beat_time_ref = htonll(server_time_ref_us);
  printf("[TEST] Time since boot big endian: %llu (%llx)\n",
         tempo_msg->beat_time_ref, tempo_msg->beat_time_ref);

  uint32_t tempo_period_us = 60 * 1000000UL / tempo;
  tempo_msg->tempo_period_us = htonl(tempo_period_us);
  printf("[TEST] Tempo period: %" PRIu32 " (%"PRIx32")\n", tempo_period_us, tempo_period_us);
  printf("[TEST] Tempo period big endian: %" PRIu32 " (%"PRIx32")\n",
         tempo_msg->tempo_period_us, tempo_msg->tempo_period_us);

  size_t data_length = sizeof(beatled_message_tempo_response_t);

  if (event_queue_add_message(event_server_message, (void *)tempo_msg,
                              data_length)) {
    puts("[ERR] Couldn't add message");
  }
}