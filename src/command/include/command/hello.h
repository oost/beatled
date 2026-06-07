#ifndef COMMAND__HELLO__HELLO_H
#define COMMAND__HELLO__HELLO_H

#include <stddef.h>

#include "beatled/protocol.h"

int send_hello_request();

int process_hello_msg(beatled_message_t *server_msg, size_t data_length);

#endif // COMMAND__HELLO__HELLO_H