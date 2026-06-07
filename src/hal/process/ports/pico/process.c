#include <pico/multicore.h>
#include <stdio.h>
#include <stdlib.h>

#include "hal/process.h"

core_loop_fn core_loop_;

void core1_boot() { core_loop_(NULL); }

void start_core1(core_loop_fn core_loop) {
  // Launch core 1
  core_loop_ = core_loop;
  multicore_launch_core1(&core1_boot);
}

void start_isr_thread(core_loop_fn isr_loop) {}

void join_cores() {}