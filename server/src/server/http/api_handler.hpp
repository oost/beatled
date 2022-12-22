#ifndef HTTP_SERVER__API_HANDLER_HPP
#define HTTP_SERVER__API_HANDLER_HPP

#include <restinio/all.hpp>

#include "beat_detector/beat_detector.hpp"
#include "core/interfaces/service_manager.hpp"
#include "core/state_manager.hpp"
#include "logger/logger.hpp"

namespace server {
class APIHandler {
public:
  using req_status_t = restinio::request_handling_status_t;
  using req_handle_t = restinio::request_handle_t;
  using route_params_t = restinio::router::route_params_t;

  APIHandler(ServiceManagerInterface &service_manager, Logger &logger);

  req_status_t on_status(const req_handle_t &req, route_params_t params);

  req_status_t on_service_control(const req_handle_t &req,
                                  route_params_t params);

  req_status_t on_tempo(const req_handle_t &req, route_params_t params);

  req_status_t on_update_program(const req_handle_t &req,
                                 route_params_t params);
  req_status_t on_log(const req_handle_t &req, route_params_t params);

  req_status_t on_preflight(const req_handle_t &req, route_params_t params);

private:
  ServiceManagerInterface &service_manager_;
  Logger &logger_;
};
};     // namespace server
#endif // HTTP_SERVER__API_HANDLER_HPP
