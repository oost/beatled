#include <atomic>

#include "status_buffer.h"

// Use atomic operations on regular shared_ptr for thread-safe updates between LED
// and render threads (C++11 compatible)
static StatusBuffer::Ptr instance_ = std::make_shared<StatusBuffer>();

StatusBuffer::Ptr StatusBuffer::load_instance() {
  return std::atomic_load(&instance_);
}

void StatusBuffer::update(const StatusData &d) {
  auto new_instance = std::make_shared<StatusBuffer>(d);
  std::atomic_store(&instance_, new_instance);
}
