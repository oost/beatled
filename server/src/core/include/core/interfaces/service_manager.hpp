#ifndef CORE__INTERFACES__SERVICE_MANAGER_HPP
#define CORE__INTERFACES__SERVICE_MANAGER_HPP

#include <fmt/format.h>

#include "../state_manager.hpp"
#include "./service_controller.hpp"

class ServiceManagerInterface {
public:
  using service_map_t = std::map<std::string, ServiceControllerInterface *>;
  ServiceControllerInterface *service(const std::string &name) {
    auto it = interface_map_.find(name);
    if (it == interface_map_.end()) {
      return nullptr;
    }
    return it->second;
  }
  const service_map_t &services() const { return interface_map_; }

  virtual ServiceControllerInterface &beat_detector() = 0;
  virtual ServiceControllerInterface &http_server() = 0;
  virtual ServiceControllerInterface &udp_server() = 0;
  virtual ServiceControllerInterface &temp_broadcaster() = 0;
  virtual StateManager &state_manager() = 0;
  virtual ~ServiceManagerInterface() {}

protected:
  void registerController(ServiceControllerInterface &controller) {
    if (service(controller.id()) != nullptr) {
      throw std::runtime_error(fmt::format(
          "A controller with this id {} already exists", controller.id()));
    }
    interface_map_[controller.id()] = &controller;
    SPDLOG_INFO("Registered controller: {}", controller.name());
  }

private:
  service_map_t interface_map_;
};

#endif // CORE__INTERFACES__SERVICE_MANAGER_HPP