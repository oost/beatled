#ifndef CORE__SERVICE_INTERFACE_H
#define CORE__SERVICE_INTERFACE_H

#include <atomic>
#include <memory>
#include <mutex>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace beatled::core {

class ServiceControllerInterface {
public:
  using Ptr = std::unique_ptr<ServiceControllerInterface>;

  ServiceControllerInterface(const std::string &id) : id_{id} {}

  virtual ~ServiceControllerInterface() =
      default; // generate a virtual default destructor

  const std::string &id() const { return id_; }

  const std::string name() const {
    return fmt::format("{} ({})", service_name(), id_);
  }

  bool is_running() { return running_; };

  void start() {
    const std::lock_guard<std::mutex> lock(status_mtx_);
    if (running_) {
      SPDLOG_INFO("{} is already running", name());
    }
    SPDLOG_INFO("Starting {}", name());
    start_sync();
    running_ = true;
  }

  void stop() {
    const std::lock_guard<std::mutex> lock(status_mtx_);
    if (!running_) {
      SPDLOG_INFO("{} is not running", name());
    }
    SPDLOG_INFO("Stopping {} server", name());
    stop_sync();
    running_ = false;
  }

protected:
  virtual void start_sync() = 0;
  virtual void stop_sync() = 0;
  virtual const char *service_name() const = 0;

  std::string id_;
  std::atomic_bool running_ = false;
  std::mutex status_mtx_;
};
} // namespace beatled::core

#endif // CORE__SERVICE_INTERFACE_H