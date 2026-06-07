#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config/constants.h"
#include "hal/registry.h"

pthread_mutex_t lock;

registry_t registry;

void registry_init() {
  if (pthread_mutex_init(&lock, NULL) != 0) {
    BEATLED_FATAL("Mutex init failed");
  }
}

void registry_lock_mutex() { pthread_mutex_lock(&lock); }

void registry_unlock_mutex() { pthread_mutex_unlock(&lock); }

bool registry_try_lock_mutex() { return !pthread_mutex_trylock(&lock); }

// pthread_mutex_destroy(&lock);
