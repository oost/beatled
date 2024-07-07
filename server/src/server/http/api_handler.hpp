#ifndef HTTP_SERVER__API_HANDLER_HPP
#define HTTP_SERVER__API_HANDLER_HPP

#include <restinio/all.hpp>

#include "./response_handler.hpp"
#include "beat_detector/beat_detector.hpp"
#include "core/interfaces/service_manager.hpp"
// #include "core/state_manager.hpp"
#include "logger/logger.hpp"

using beatled::core::ServiceManagerInterface;

namespace beatled {
namespace server {
class APIHandler : public ResponseHandler {
public:
  using req_status_t = restinio::request_handling_status_t;
  using req_handle_t = restinio::request_handle_t;
  using route_params_t = restinio::router::route_params_t;

  APIHandler(ServiceManagerInterface &service_manager, Logger &logger);

  req_status_t on_get_status(const req_handle_t &req, route_params_t params);

  req_status_t on_post_service_control(const req_handle_t &req,
                                       route_params_t params);

  req_status_t on_get_tempo(const req_handle_t &req, route_params_t params);

  req_status_t on_get_program(const req_handle_t &req, route_params_t params);
  req_status_t on_post_program(const req_handle_t &req, route_params_t params);
  req_status_t on_get_log(const req_handle_t &req, route_params_t params);

  req_status_t on_preflight(const req_handle_t &req, route_params_t params);

private:
  template <typename RESP> RESP init_resp(RESP resp) {
    return ResponseHandler::init_resp<RESP>(std::forward<RESP>(resp))
        .append_header(restinio::http_field::content_type,
                       "text/json; charset=utf-8")
        .append_header(restinio::http_field::access_control_allow_origin, "*")
        .append_header(restinio::http_field::access_control_allow_headers,
                       "Origin, X-Requested-With, Content-Type, Accept");
  }

  ServiceManagerInterface &service_manager_;
  Logger &logger_;
};
} // namespace server
} // namespace beatled
#endif // HTTP_SERVER__API_HANDLER_HPP
