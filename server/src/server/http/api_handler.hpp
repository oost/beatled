#ifndef HTTP_SERVER__API_HANDLER_HPP
#define HTTP_SERVER__API_HANDLER_HPP

#include <restinio/all.hpp>

#include "beat_detector/beat_detector.hpp"
#include "logger/logger.hpp"
#include "state_manager/state_manager.hpp"

namespace server {
class APIHandler {
public:
  using req_status_t = restinio::request_handling_status_t;
  using req_handle_t = restinio::request_handle_t;
  using route_params_t = restinio::router::route_params_t;

  APIHandler(StateManager &state_manager, Logger &logger,
             beat_detector::BeatDetector &beat_detector);

  req_status_t on_status(const req_handle_t &req, route_params_t params);
  req_status_t on_beat_detector_start(const req_handle_t &req,
                                      route_params_t params);
  req_status_t on_beat_detector_stop(const req_handle_t &req,
                                     route_params_t params);
  req_status_t on_beat_detector_status(const req_handle_t &req,
                                       route_params_t params);
  req_status_t on_tempo(const req_handle_t &req, route_params_t params);

  req_status_t on_update_program(const req_handle_t &req,
                                 route_params_t params);
  req_status_t on_log(const req_handle_t &req, route_params_t params);

private:
  StateManager &state_manager_;
  Logger &logger_;
  beat_detector::BeatDetector &beat_detector_;
};
};     // namespace server
#endif // HTTP_SERVER__API_HANDLER_HPP
