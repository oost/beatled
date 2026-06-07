#include <time.h>
#include <unistd.h>

#include "hal/process.h"

void sleep_ms(uint32_t duration) { usleep(duration * 1000); }