#ifndef CORE__SERVICE_INTERFACE_H
#define CORE__SERVICE_INTERFACE_H

#include <atomic>
#include <mutex>
#include <spdlog/spdlog.h>

class ServiceControllerInterface {
public:
  ServiceControllerInterface(const std::string name) : name_{name} {}
  virtual ~ServiceControllerInterface() =
      default; // generate a virtual default destructor
  bool is_running() { return running_; };
  void start() {
    const std::lock_guard<std::mutex> lock(status_mtx_);
    if (running_) {
      SPDLOG_INFO("{} is already running", name_);
    }
    SPDLOG_INFO("Starting {}", name_);
    start_sync();
    running_ = true;
  }
  void stop() {
    const std::lock_guard<std::mutex> lock(status_mtx_);
    if (!running_) {
      SPDLOG_INFO("{} is not running", name_);
    }
    SPDLOG_INFO("Stopping {} server", name_);
    stop_sync();
    running_ = false;
  }

protected:
  virtual void start_sync() = 0;
  virtual void stop_sync() = 0;
  std::string name_;
  std::atomic_bool running_ = false;
  std::mutex status_mtx_;
};

#endif // CORE__SERVICE_INTERFACE_H