#ifndef BEATLED_PROTOCOL__SERIALIZE_H
#define BEATLED_PROTOCOL__SERIALIZE_H

#include "protocol.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef int (*process_message_fn_t)(void *data);

// const struct {
//     beatled_message_type_t type;
//     process_message_fn_t process_fn;
// } process_fn[] = {
//         {pattern_snakes,  "Snakes!"},
//         {pattern_random,  "Random data"},
//         {pattern_sparkle, "Sparkles"},
//         {pattern_greys,   "Greys"},
// //        {pattern_solid,  "Solid!"},
// //        {pattern_fade, "Fade"},
// };

static inline bool check_size(size_t data_length, size_t expected_length) {
  if (data_length != expected_length) {
    printf("Sizes don't match %zu <> %zu\n", data_length, expected_length);
    return false;
  }
  return true;
}

static inline int deserialize_message(beatled_message_t *server_msg,
                                  size_t data_length,
                                  process_message_fn_t *process_fn_map,
                                  size_t process_fn_map_length);

#endif // BEATLED_PROTOCOL__SERIALIZE_H