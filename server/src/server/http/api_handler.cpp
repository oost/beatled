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

  json status;
  for (auto &&[first, second] : service_manager_.services()) {
    status[second->id()] = second->is_running();
  }
  j["status"] = status;
  j["tempo"] = service_manager_.state_manager().get_tempo_ref().tempo;

  init_resp(req->create_response())
      .append_header(restinio::http_field::content_type,
                     "text/json; charset=utf-8")
      .set_body(j.dump())
      .done();

  return restinio::request_accepted();
}

APIHandler::req_status_t APIHandler::on_service_control(const req_handle_t &req,
                                                        route_params_t params) {
  json response_body;

  try {
    json request_body = json::parse(req->body());
    std::string service_name = request_body["id"].get<std::string>();
    bool requested_status = request_body["status"].get<bool>();
    ServiceControllerInterface *service =
        service_manager_.service(service_name);

    if (service) {
      if (requested_status) {
        service->start();
      } else {
        service->stop();
      }
      auto resp = init_resp(req->create_response())
                      .append_header(restinio::http_field::content_type,
                                     "text/json; charset=utf-8");
      response_body["status"] = service->is_running();

      resp.set_body(response_body.dump()).done();
      return restinio::request_accepted();

    } else {
      response_body["error"] = "Not found";

      return req->create_response(restinio::status_not_found())
          .append_header_date_field()
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
  } catch (const std::exception &e) {
    response_body["error"] = "Error";

    SPDLOG_ERROR("Error with request: {}", e.what());
    return req->create_response(restinio::status_bad_request())
        .append_header_date_field()
        .set_body(response_body.dump())
        .connection_close()
        .done();
  }
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