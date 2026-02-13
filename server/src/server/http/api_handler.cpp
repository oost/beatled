#include <nlohmann/json.hpp>

#include "./api_handler.hpp"

using json = nlohmann::json;
using beatled::core::tempo_ref_t;

namespace beatled::server {

struct Program {
  std::string name;
  int id;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Program, name, id)

APIHandler::APIHandler(ServiceManagerInterface &service_manager, Logger &logger,
                       const std::string &cors_origin,
                       const std::string &api_token)
    : service_manager_{service_manager}, logger_{logger},
      cors_origin_{cors_origin}, api_token_{api_token} {}

bool APIHandler::check_auth(const req_handle_t &req) const {
  if (api_token_.empty())
    return true;
  auto auth = req->header().opt_value_of(restinio::http_field::authorization);
  return auth && *auth == "Bearer " + api_token_;
}

APIHandler::req_status_t APIHandler::on_get_status(const req_handle_t &req,
                                                   route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }

  json response_body;
  response_body["message"] = "It's all good!";

  json service_status;
  for (auto &&[first, second] : service_manager_.services()) {
    service_status[second->id()] = second->is_running();
  }
  response_body["status"] = service_status;
  response_body["tempo"] =
      service_manager_.state_manager().get_tempo_ref().tempo;

  return init_resp(req->create_response(restinio::status_ok()))
      .set_body(response_body.dump())
      .done();
}

APIHandler::req_status_t
APIHandler::on_post_service_control(const req_handle_t &req,
                                    route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }

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

      return init_resp(req->create_response(restinio::status_ok()))
          .set_body(response_body.dump())
          .done();

    } else {
      response_body["error"] = "Not found";

      return init_resp(req->create_response(restinio::status_not_found()))
          .set_body(response_body.dump())
          .connection_close()
          .done();
    }
  } catch (const std::exception &e) {
    response_body["error"] = e.what();

    SPDLOG_ERROR("Error with request: {}", e.what());
    return init_resp(req->create_response(restinio::status_bad_request()))
        .set_body(response_body.dump())
        .connection_close()
        .done();
  }
}

APIHandler::req_status_t APIHandler::on_get_tempo(const req_handle_t &req,
                                                  route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }

  tempo_ref_t tr = service_manager_.state_manager().get_tempo_ref();

  json response_body;

  response_body["tempo"] = tr.tempo;
  response_body["time_ref"] = tr.beat_time_ref;

  return init_resp(req->create_response(restinio::status_ok()))
      .set_body(response_body.dump())
      .done();
}

APIHandler::req_status_t APIHandler::on_post_program(const req_handle_t &req,
                                                     route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }

  json response_body;
  try {
    json request_body = json::parse(req->body());
    uint16_t program_id = request_body["programId"].get<std::uint16_t>();
    service_manager_.state_manager().update_program_id(program_id);

    response_body["message"] = fmt::format("Updated program to {}", program_id);

    return init_resp(req->create_response(restinio::status_ok()))
        .set_body(response_body.dump())
        .done();
  } catch (const std::exception &e) {
    response_body["error"] = e.what();

    SPDLOG_ERROR("Error with request: {}", e.what());
    return init_resp(req->create_response(restinio::status_bad_request()))
        .set_body(response_body.dump())
        .connection_close()
        .done();
  }
}

APIHandler::req_status_t APIHandler::on_get_program(const req_handle_t &req,
                                                    route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }

  uint16_t program_id = service_manager_.state_manager().get_program_id();

  json response_body;
  response_body["message"] = fmt::format("Current program is {}", program_id);

  response_body["programs"] = json::array(
      {Program{"Snakes!", 0}, Program{"Random data", 1}, Program{"Sparkles", 2},
       Program{"Greys", 3}, Program{"Drops", 4}, Program{"Solid!", 5},
       Program{"Fade", 6}, Program{"Fade Color", 7}});

  response_body["programId"] = program_id;

  return init_resp(req->create_response(restinio::status_ok()))
      .set_body(response_body.dump())
      .done();
}

APIHandler::req_status_t APIHandler::on_get_log(const req_handle_t &req,
                                                route_params_t params) {
  if (!check_auth(req)) {
    return init_resp(req->create_response(restinio::status_unauthorized()))
        .set_body(R"({"error":"Unauthorized"})")
        .done();
  }

  return init_resp(req->create_response(restinio::status_ok()))
      .set_body(logger_.log_tail().dump())
      .done();
}

APIHandler::req_status_t APIHandler::on_preflight(const req_handle_t &req,
                                                  route_params_t params) {
  return init_resp(req->create_response(restinio::status_ok())).done();
}

} // namespace beatled::server
