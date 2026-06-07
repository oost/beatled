#include "hal/startup.h"

extern void start_beatled();

void app_main(void) {
  startup(&start_beatled);
}
