
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <restinio/all.hpp>

#include "file_extensions.hpp"
#include "http_server.hpp"

using namespace server;

using json = nlohmann::json;
using namespace server;

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

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

  // router->add_handler(http_method_head(), route_path, std::move(handler));

  router->http_get("/api/status", [](auto req, auto) {
    // create an empty structure (null)
    json j;

    // to an object)
    j["message"] = "It's all good!";

    init_resp(req->create_response())
        .append_header(restinio::http_field::content_type,
                       "text/json; charset=utf-8")
        .set_body(j.dump())
        .done();

    return restinio::request_accepted();
  });

  router->http_get("/api/tempo", [this](auto req, auto) {
    // create an empty structure (null)
    state_manager_.post_tempo(
        [req](float tempo, uint64_t tv_sec, uint32_t tv_nsec) {
          json j;

          // to an object)
          j["tempo"] = tempo;
          j["tv_sec"] = tv_sec;
          j["tv_nsec"] = tv_nsec;

          init_resp(req->create_response())
              .append_header(restinio::http_field::content_type,
                             "text/json; charset=utf-8")
              .set_body(j.dump())
              .done();
        });

    return restinio::request_accepted();
  });

  router->http_post(
      "/api/update-program", [](const auto &req, const auto &params) {
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
      });

  router->http_get("/api/log", [this](const auto &req, const auto &params) {
    init_resp(req->create_response())
        .append_header(restinio::http_field::content_type,
                       "text/json; charset=utf-8")
        .set_body(logger_.log_tail().dump())
        .done();

    return restinio::request_accepted();
  });

  // GET request to homepage.
  router->http_get(
      R"(/:path(.*)\.:ext(.*))", restinio::path2regex::options_t{}.strict(true),
      [server_root_dir](auto req, auto params) {
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
                .append_header(restinio::http_field::expires,
                               std::move(expires_at))
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
      });

  // GET request to homepage.
  router->http_get("/", [](auto req, auto) {
    return req->create_response(restinio::status_temporary_redirect())
        .append_header(restinio::http_field::location, "/index.html")
        .append_header_date_field()
        .connection_close()
        .done();
  });

  router->non_matched_request_handler([](auto req) {
    if (restinio::http_method_get() == req->header().method())
      return req->create_response(restinio::status_not_found())
          .append_header_date_field()
          .connection_close()
          .done();

    return req->create_response(restinio::status_not_implemented())
        .append_header_date_field()
        .connection_close()
        .done();
  });

  return router;
}

HTTPServer::HTTPServer(asio::io_context &io_context,
                       const http_server_parameters_t &http_server_parameters,
                       StateManager &state_manager, Logger &logger)
    : state_manager_{state_manager}, logger_{logger} {
  using namespace std::chrono;

  std::cout << "Starting HTTP server, listening on: "
            << http_server_parameters.address << ":"
            << http_server_parameters.port << std::endl;

  using traits_t =
      restinio::traits_t<restinio::asio_timer_manager_t,
                         // restinio::null_logger_t,
                         restinio::single_threaded_ostream_logger_t, router_t>;

  restinio::run(io_context, restinio::on_this_thread<traits_t>()
                                .address(http_server_parameters.address)
                                .port(http_server_parameters.port)
                                .request_handler(server_handler(
                                    http_server_parameters.root_dir))
                                .read_next_http_message_timelimit(10s)
                                .write_http_response_timelimit(1s)
                                .handle_request_timeout(1s));
}
