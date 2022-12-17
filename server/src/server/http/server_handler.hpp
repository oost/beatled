#ifndef HTTP_SERVER_SERVER_HANDLER_HPP
#define HTTP_SERVER_SERVER_HANDLER_HPP

#include <memory>
#include <string>
#include <utility>

#include <fmt/std.h>
#include <nlohmann/json.hpp>
#include <restinio/all.hpp>
#include <spdlog/spdlog.h>

#include "./file_extensions.hpp"
#include "http_server/http_server.hpp"

using json = nlohmann::json;
namespace server {

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

class HTTPServerHandler {
public:
  explicit HTTPServerHandler() : {}

  HTTPServerHandler(const HTTPServerHandler &) = delete;
  HTTPServerHandler(HTTPServerHandler &&) = delete;

  auto on_status(const restinio::request_handle_t &req,
                 rr::route_params_t params) {
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

  auto on_beat_detector_start(const restinio::request_handle_t &req,
                              rr::route_params_t params) {
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

  auto on_beat_detector_stop(const restinio::request_handle_t &req,
                             rr::route_params_t params) {
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

  auto on_beat_detector_status(const restinio::request_handle_t &req,
                               rr::route_params_t params) {
    json j;
    j["is_running"] = beat_detector_.is_running();

    init_resp(req->create_response())
        .append_header(restinio::http_field::content_type,
                       "text/json; charset=utf-8")
        .set_body(j.dump())
        .done();
    return restinio::request_accepted();
  }

  auto on_beat_tempo(const restinio::request_handle_t &req,
                     rr::route_params_t params) {
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

  auto on_update_program(const restinio::request_handle_t &req,
                         rr::route_params_t params) {
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

  auto on_log(const restinio::request_handle_t &req,
              rr::route_params_t params) {
    init_resp(req->create_response())
        .append_header(restinio::http_field::content_type,
                       "text/json; charset=utf-8")
        .set_body(logger_.log_tail().dump())
        .done();

    return restinio::request_accepted();
  }

  // GET request to homepage.
  auto on_file_get(const restinio::request_handle_t &req,
                   rr::route_params_t params) {
    auto path = req->header().path();

    if (std::string::npos == path.find("..")) {
      // A nice path.

      const auto file_path =
          server_root_dir + std::string{path.data(), path.size()};

      try {
        auto sf = restinio::sendfile(file_path);
        auto modified_at =
            restinio::make_date_field_value(sf.meta().last_modified_at());

        auto expires_at = restinio::make_date_field_value(
            std::chrono::system_clock::now() + std::chrono::hours(24 * 7));

        return req->create_response()
            .append_header(restinio::http_field::server, "RESTinio")
            .append_header_date_field()
            .append_header(restinio::http_field::last_modified,
                           std::move(modified_at))
            .append_header(restinio::http_field::expires, std::move(expires_at))
            .append_header(restinio::http_field::content_type,
                           content_type_by_file_extention(params["ext"]))
            .set_body(std::move(sf))
            .done();
      } catch (const std::exception &) {
        return req->create_response(restinio::status_not_found())
            .append_header_date_field()
            .connection_close()
            .done();
      }
    } else {
      // Bad path.
      return req->create_response(restinio::status_forbidden())
          .append_header_date_field()
          .connection_close()
          .done();
    }
  }

  // GET request to homepage.
  auto on_root_get(const restinio::request_handle_t &req,
                   rr::route_params_t params) {
    return req->create_response(restinio::status_temporary_redirect())
        .append_header(restinio::http_field::location, "/index.html")
        .append_header_date_field()
        .connection_close()
        .done();
  }

  auto on_non_matched(const restinio::request_handle_t &req) {
    if (restinio::http_method_get() == req->header().method())
      return req->create_response(restinio::status_not_found())
          .append_header_date_field()
          .connection_close()
          .done();

    return req->create_response(restinio::status_not_implemented())
        .append_header_date_field()
        .connection_close()
        .done();
  }

private:
  template <typename RESP> static RESP init_resp(RESP resp) {
    resp.append_header("Server", "RESTinio sample server /v.0.2")
        .append_header_date_field()
        .append_header("Content-Type", "text/plain; charset=utf-8");

    return resp;
  }

  template <typename RESP> static void mark_as_bad_request(RESP &resp) {
    resp.header().status_line(restinio::status_bad_request());
  }
};

auto HTTPServer::server_handler(const std::string &root_dir) {
  auto router = std::make_unique<router_t>();

  std::string server_root_dir;

  if (root_dir.empty()) {
    server_root_dir = "./";
  } else if (root_dir.back() != '/' && root_dir.back() != '\\') {
    server_root_dir = root_dir + '/';
  } else {
    server_root_dir = root_dir;
  }

  auto handler = std::make_shared<HTTPServerHandler>();

  auto by = [&](auto method) {
    using namespace std::placeholders;
    return std::bind(method, handler, _1, _2);
  };

  // router->add_handler(http_method_head(), route_path, std::move(handler));
  router->http_get("/api/status", by(&HTTPServerHandler::on_status));

  router->http_get("/api/beat-detector/start",
                   by(&HTTPServerHandler::on_beat_detector_start));

  router->http_get("/api/beat-detector/stop",
                   by(&HTTPServerHandler::on_beat_detector_stop));

  router->http_get("/api/beat-detector/status",
                   by(&HTTPServerHandler::on_beat_detector_status));

  router->http_get("/api/tempo", by(&HTTPServerHandler::on_tempo));

  router->http_post("/api/update-program",
                    by(&HTTPServerHandler::on_update_program));

  router->http_get("/api/log", by(&HTTPServerHandler::on_log));

  // GET request to homepage.
  router->http_get(R"(/:path(.*)\.:ext(.*))",
                   restinio::path2regex::options_t{}.strict(true),
                   by(&HTTPServerHandler::on_file_get));

  // GET request to homepage.
  router->http_get("/", by(&HTTPServerHandler::on_root_get));

  router->non_matched_request_handler(by(&HTTPServerHandler::on_non_matched));

  return router;
}

} // namespace server

#endif // HTTP_SERVER_SERVER_HANDLER_HPP