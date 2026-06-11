#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "hal/process.h"

pthread_t core1_thread;

void start_core1(core_loop_fn core_loop) {
  pthread_create(&core1_thread, NULL, core_loop, NULL);
}

void join_cores() {
  pthread_join(core1_thread, NULL);
}