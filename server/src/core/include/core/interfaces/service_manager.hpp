#ifndef CORE__INTERFACES__SERVICE_MANAGER_HPP
#define CORE__INTERFACES__SERVICE_MANAGER_HPP

#include "../state_manager.hpp"
#include "./service_controller.hpp"

class ServiceManagerInterface {
public:
  virtual ServiceControllerInterface &beat_detector() = 0;
  virtual ServiceControllerInterface &http_server() = 0;
  virtual ServiceControllerInterface &udp_server() = 0;
  virtual ServiceControllerInterface &temp_broadcaster() = 0;
  virtual StateManager &state_manager() = 0;
  virtual ~ServiceManagerInterface() {}
};

#endif // CORE__INTERFACES__SERVICE_MANAGER_HPP