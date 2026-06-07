#include "state_manager/states/registered.h"
#include "command/command.h"

int enter_registered_state() {
  send_time_request();
  return 0;
}
int exit_registered_state() { return 0; }