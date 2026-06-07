#ifndef SRC__EVENT__EVENT__H_
#define SRC__EVENT__EVENT__H_

#include <stdint.h>

typedef enum {
  event_error = 0,
  event_server_message,
  event_state_transition
} event_type_t;

typedef struct {
  event_type_t event_type;
  uint64_t time;
  size_t data_length;
  void *data;
} event_t;

#endif // SRC__EVENT__EVENT__H_
