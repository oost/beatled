#ifndef CORE__INTERFACES__SERVICE_MANAGER_HPP
#define CORE__INTERFACES__SERVICE_MANAGER_HPP

#include <fmt/format.h>
#include <stdexcept>

#include "../state_manager.hpp"
#include "./service_controller.hpp"

namespace beatled::core {

class ServiceManagerInterface {
public:
  using service_map_t = std::map<std::string, ServiceControllerInterface::Ptr>;

  ServiceControllerInterface *service(const std::string &name) {
    auto it = interface_map_.find(name);
    if (it == interface_map_.end()) {
      return nullptr;
    }
    return it->second.get();
  }
  const service_map_t &services() const { return interface_map_; }

  virtual StateManager &state_manager() = 0;
  virtual ~ServiceManagerInterface() {}

protected:
  void registerController(ServiceControllerInterface::Ptr controller) {
    if (service(controller->id()) != nullptr) {
      throw std::invalid_argument(
          fmt::format("A controller with this id {} already exists", controller->id()));
    }
    SPDLOG_INFO("Registering controller: {}", controller->name());
    interface_map_[controller->id()] = std::move(controller);
  }

private:
  service_map_t interface_map_;
};

} // namespace beatled::core

#endif // CORE__INTERFACES__SERVICE_MANAGER_HPP