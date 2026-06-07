#include <stdint.h>
#include <stdio.h>

#include "protocol.h"

uint16_t beatled_message_sizes[BEATLED_MESSAGE_LAST_VALUE - 1];

// beatled_message_sizes[BEATLED_MESSAGE_ERROR] =
//     (uint16_t)sizeof(beatled_message_error_t);

// beatled_message_sizes[BEATLED_MESSAGE_TEMPO_REQUEST] =
//     (uint16_t)sizeof(beatled_message_tempo_request_t);

// beatled_message_sizes[BEATLED_MESSAGE_TEMPO_RESPONSE] =
//     (uint16_t)sizeof(beatled_message_tempo_response_t);

// beatled_message_sizes[BEATLED_MESSAGE_HELLO_REQUEST] =
//     (uint16_t)sizeof(beatled_message_hello_request_t);

// beatled_message_sizes[BEATLED_MESSAGE_HELLO_RESPONSE] =
//     (uint16_t)sizeof(beatled_message_hello_response_t);

// beatled_message_sizes[BEATLED_MESSAGE_TIME_REQUEST] =
//     (uint16_t)sizeof(beatled_message_time_request_t);

// beatled_message_sizes[BEATLED_MESSAGE_TIME_RESPONSE] =
//     (uint16_t)sizeof(beatled_message_time_response_t);

// beatled_message_sizes[BEATLED_MESSAGE_PROGRAM] =
//     (uint16_t)sizeof(beatled_message_program_t);

// bool check_size(size_t data_length, beatled_message_type_t msg_type) {
//   if (msg_type >= BEATLED_MESSAGE_LAST_VALUE) {
//     return false;
//   }
//   if (data_length != beatled_message_sizes[msg_type]) {
//     printf("Sizes don't match %zu <> %zu\n", data_length, expected_length);
//     return false;
//   }
//   return true;
// }