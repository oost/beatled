#ifndef SRC__COMMAND__NEXT_BEAT__NEXT_BEAT__H_
#define SRC__COMMAND__NEXT_BEAT__NEXT_BEAT__H_

#include <stdint.h>

#include "beatled/protocol.h"

int process_next_beat_msg(beatled_message_t *server_msg, size_t data_length);

// Total NEXT_BEAT broadcasts the controller knows it missed, derived from
// the protocol-v2 sequence number gaps. Exposed for status/log heartbeats
// and integration tests.
uint32_t next_beat_get_gap_total(void);

#endif // SRC__COMMAND__NEXT_BEAT__NEXT_BEAT__H_