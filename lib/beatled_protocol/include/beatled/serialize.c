#include "serialize.h"

int error_fn(void *data) { return 1; }

static inline int deserialize_message(beatled_message_t *msg_data,
                                      size_t data_length,
                                      process_message_fn_t *process_fn_map,
                                      size_t process_fn_map_length) {
  if (data_length < 1) {
    return 1;
  }
  beatled_message_type_t message_type = msg_data->type;

  deserialize_fn = (message_type < process_fn_map_length)
                       ? message_map[message_type]
                       : &error_fn;
  process_fn = (message_type < process_fn_map_length)
                   ? process_fn_map[message_type]
                   : &error_fn;

  return deserialize_fn(msg_data, data_length, process_fn);
}
