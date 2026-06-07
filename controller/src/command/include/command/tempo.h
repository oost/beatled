#ifndef COMMAND__TEMPO__TEMPO_H
#define COMMAND__TEMPO__TEMPO_H

#include <stdlib.h>

#include "beatled/protocol.h"

int send_tempo_request();

int process_tempo_msg(beatled_message_t *server_msg, size_t data_length);

#endif // COMMAND__TEMPO__TEMPO_H