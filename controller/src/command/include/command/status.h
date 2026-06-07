#ifndef COMMAND__STATUS_H
#define COMMAND__STATUS_H

#include <stddef.h>

#include "beatled/protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

// Handle a server-initiated STATUS_REQUEST (protocol v4) by replying
// with a STATUS_RESPONSE that echoes the server's send timestamp and
// carries the current diagnostic snapshot built by qos_block_fill().
int process_status_request(beatled_message_t *server_msg, size_t data_length);

#ifdef __cplusplus
}
#endif

#endif // COMMAND__STATUS_H
