#include <nlohmann/json.hpp>

#include "./api_handler.hpp"
#include "./file_extensions.hpp"

using json = nlohmann::json;
using namespace server;

APIHandler::APIHandler(StateManager &state_manager, Logger &logger,
                       beat_detector::BeatDetector &beat_detector)
    : state_manager_{state_manager}, logger_{logger}, beat_detector_{
                                                          beat_detector} {}

APIHandler::req_status_t APIHandler::on_status(const req_handle_t &req,
                                               route_params_t params) {
  // create an empty structure (null)
  json j;

  // to an object)
  j["message"] = "It's all good!";
  j["http_server"] = false;
  j["udp_server"] = false;
  j["broadcaster"] = false;
  j["beat_detector"] = beat_detector_.is_running();
  j["tempo"] = state_manager_.get_tempo_ref().tempo;

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
  beat_detector_.run();
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
  beat_detector_.request_stop();
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
  j["is_running"] = beat_detector_.is_running();

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
  tempo_ref_t tr = state_manager_.get_tempo_ref();

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