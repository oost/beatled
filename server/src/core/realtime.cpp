#include "core/realtime.hpp"

#include <spdlog/spdlog.h>

#if defined(__linux__)
#include <cerrno>
#include <cstring>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#endif // defined(__linux__)

namespace beatled::core {

#if defined(__linux__)

bool lock_process_memory() {
  if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
    SPDLOG_WARN("mlockall(MCL_CURRENT | MCL_FUTURE) failed: {} — continuing with "
                "pageable memory (set CAP_IPC_LOCK / raise RLIMIT_MEMLOCK to enable)",
                std::strerror(errno));
    return false;
  }
  SPDLOG_INFO("Locked process memory (mlockall MCL_CURRENT | MCL_FUTURE)");
  return true;
}

bool set_thread_realtime_priority(int priority) {
  struct sched_param param = {};
  param.sched_priority = priority;

  if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
    SPDLOG_WARN("pthread_setschedparam(SCHED_FIFO, prio={}) failed: {} — thread stays "
                "on SCHED_OTHER (run as root or grant CAP_SYS_NICE to enable)",
                priority, std::strerror(errno));
    return false;
  }
  SPDLOG_INFO("Thread promoted to SCHED_FIFO priority {}", priority);
  return true;
}

#else // !defined(__linux__)

bool lock_process_memory() {
  SPDLOG_DEBUG("lock_process_memory(): no-op on non-Linux platform");
  return false;
}

bool set_thread_realtime_priority(int /*priority*/) {
  SPDLOG_DEBUG("set_thread_realtime_priority(): no-op on non-Linux platform");
  return false;
}

#endif // defined(__linux__)

} // namespace beatled::core
