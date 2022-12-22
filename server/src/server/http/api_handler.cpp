#include <nlohmann/json.hpp>

#include "./api_handler.hpp"
#include "./file_extensions.hpp"
#include "./utils.hpp"

using json = nlohmann::json;
using namespace server;

APIHandler::APIHandler(ServiceManagerInterface &service_manager, Logger &logger)
    : service_manager_{service_manager}, logger_{logger} {}

APIHandler::req_status_t APIHandler::on_status(const req_handle_t &req,
                                               route_params_t params) {
  // create an empty structure (null)
  json response_body;

  // to an object)
  response_body["message"] = "It's all good!";

  json service_status;
  for (auto &&[first, second] : service_manager_.services()) {
    service_status[second->id()] = second->is_running();
  }
  response_body["status"] = service_status;
  response_body["tempo"] =
      service_manager_.state_manager().get_tempo_ref().tempo;

  return init_resp_cors(req->create_response(restinio::status_ok()))
      .append_header(restinio::http_field::content_type,
                     "text/json; charset=utf-8")
      .set_body(response_body.dump())
      .done();
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
      response_body["status"] = service->is_running();

      return init_resp_cors(req->create_response(restinio::status_ok()))
          .append_header(restinio::http_field::content_type,
                         "text/json; charset=utf-8")
          .set_body(response_body.dump())
          .done();

    } else {
      response_body["error"] = "Not found";

      return init_resp_cors(req->create_response(restinio::status_not_found()))
          .append_header_date_field()
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
  } catch (const std::exception &e) {
    response_body["error"] = "Error";

    SPDLOG_ERROR("Error with request: {}", e.what());
    return init_resp_cors(req->create_response(restinio::status_bad_request()))
        .append_header_date_field()
        .set_body(response_body.dump())
        .connection_close()
        .done();
  }
}

APIHandler::req_status_t APIHandler::on_tempo(const req_handle_t &req,
                                              route_params_t params) {
  tempo_ref_t tr = service_manager_.state_manager().get_tempo_ref();

  json response_body;

  response_body["tempo"] = tr.tempo;
  response_body["time_ref"] = tr.beat_time_ref;

  return init_resp_cors(req->create_response(restinio::status_ok()))
      .append_header(restinio::http_field::content_type,
                     "text/json; charset=utf-8")
      .set_body(response_body.dump())
      .done();
}

APIHandler::req_status_t APIHandler::on_update_program(const req_handle_t &req,
                                                       route_params_t params) {
  // create an empty structure (null)
  json request_body = json::parse(req->body());

  json response_body;
  // to an object)
  response_body["message"] =
      "Update program to " + request_body["programId"].get<std::string>();

  return init_resp_cors(req->create_response(restinio::status_ok()))
      .append_header(restinio::http_field::content_type,
                     "text/json; charset=utf-8")
      .set_body(response_body.dump())
      .done();
}

APIHandler::req_status_t APIHandler::on_log(const req_handle_t &req,
                                            route_params_t params) {
  return init_resp_cors(req->create_response(restinio::status_ok()))
      .append_header(restinio::http_field::content_type,
                     "text/json; charset=utf-8")
      .set_body(logger_.log_tail().dump())
      .done();
}

APIHandler::req_status_t APIHandler::on_preflight(const req_handle_t &req,
                                                  route_params_t params) {
  return init_resp_cors(req->create_response(restinio::status_ok()))
      .append_header(restinio::http_field::content_type,
                     "text/json; charset=utf-8")
      .done();
}