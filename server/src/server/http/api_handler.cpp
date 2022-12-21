#include <nlohmann/json.hpp>

#include "./api_handler.hpp"
#include "./file_extensions.hpp"

using json = nlohmann::json;
using namespace server;

APIHandler::APIHandler(ServiceManagerInterface &service_manager, Logger &logger)
    : service_manager_{service_manager}, logger_{logger} {}

APIHandler::req_status_t APIHandler::on_status(const req_handle_t &req,
                                               route_params_t params) {
  // create an empty structure (null)
  json j;

  // to an object)
  j["message"] = "It's all good!";
  j["http_server"] = service_manager_.http_server().is_running();
  j["udp_server"] = service_manager_.udp_server().is_running();
  j["broadcaster"] = service_manager_.temp_broadcaster().is_running();
  j["beat_detector"] = service_manager_.beat_detector().is_running();

  j["tempo"] = service_manager_.state_manager().get_tempo_ref().tempo;

  init_resp(req->create_response())
      .append_header(restinio::http_field::content_type,
                     "text/json; charset=utf-8")
      .set_body(j.dump())
      .done();

  return restinio::request_accepted();
}

APIHandler::req_status_t
APIHandler::on_beat_detector_start(const req_handle_t &req,
                                   route_params_t params) {
  service_manager_.beat_detector().start();
  json j;
  j["OK"] = true;

  init_resp(req->create_response())
      .append_header(restinio::http_field::content_type,
                     "text/json; charset=utf-8")
      .set_body(j.dump())
      .done();
  return restinio::request_accepted();
}

APIHandler::req_status_t
APIHandler::on_beat_detector_stop(const req_handle_t &req,
                                  route_params_t params) {
  service_manager_.beat_detector().stop();
  json j;
  j["OK"] = true;

  init_resp(req->create_response())
      .append_header(restinio::http_field::content_type,
                     "text/json; charset=utf-8")
      .set_body(j.dump())
      .done();
  return restinio::request_accepted();
}

APIHandler::req_status_t
APIHandler::on_beat_detector_status(const req_handle_t &req,
                                    route_params_t params) {
  json j;
  j["is_running"] = service_manager_.beat_detector().is_running();

  init_resp(req->create_response())
      .append_header(restinio::http_field::content_type,
                     "text/json; charset=utf-8")
      .set_body(j.dump())
      .done();
  return restinio::request_accepted();
}

APIHandler::req_status_t APIHandler::on_tempo(const req_handle_t &req,
                                              route_params_t params) {
  // create an empty structure (null)
  tempo_ref_t tr = service_manager_.state_manager().get_tempo_ref();

  json j;

  // to an object)
  j["tempo"] = tr.tempo;
  j["time_ref"] = tr.beat_time_ref;
  // j["tv_sec"] = tv_sec;
  // j["tv_nsec"] = tv_nsec;

  init_resp(req->create_response())
      .append_header(restinio::http_field::content_type,
                     "text/json; charset=utf-8")
      .set_body(j.dump())
      .done();

  return restinio::request_accepted();
}

APIHandler::req_status_t APIHandler::on_update_program(const req_handle_t &req,
                                                       route_params_t params) {
  // create an empty structure (null)
  json body = json::parse(req->body());

  json resp_body;
  // to an object)
  resp_body["message"] =
      "Update program to " + body["programId"].get<std::string>();

  init_resp(req->create_response())
      .append_header(restinio::http_field::content_type,
                     "text/json; charset=utf-8")
      .set_body(resp_body.dump())
      .done();

  return restinio::request_accepted();
}

APIHandler::req_status_t APIHandler::on_log(const req_handle_t &req,
                                            route_params_t params) {
  init_resp(req->create_response())
      .append_header(restinio::http_field::content_type,
                     "text/json; charset=utf-8")
      .set_body(logger_.log_tail().dump())
      .done();

  return restinio::request_accepted();
}