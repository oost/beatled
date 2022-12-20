#include <filesystem>
#include <iostream>
#include <map>
#include <memory>

#include <fmt/std.h>
#include <nlohmann/json.hpp>
#include <restinio/all.hpp>
#include <restinio/tls.hpp>
#include <spdlog/spdlog.h>

#include "./api_handler.hpp"
#include "./file_extensions.hpp"
#include "./file_handler.hpp"
#include "http_server/http_server.hpp"

using json = nlohmann::json;
using server::HTTPServer;

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

auto HTTPServer::server_handler(const std::string &root_dir) {
  auto router = std::make_unique<router_t>();

  auto file_handler = std::make_shared<FileHandler>(root_dir);

  auto by_file_handler = [&](auto method) {
    using namespace std::placeholders;
    return std::bind(method, file_handler, _1, _2);
  };

  auto api_handler =
      std::make_shared<APIHandler>(state_manager_, logger_, beat_detector_);
  auto by_api_handler = [&](auto method) {
    using namespace std::placeholders;
    return std::bind(method, api_handler, _1, _2);
  };

  router->http_get("/api/status", by_api_handler(&APIHandler::on_status));

  router->http_get("/api/beat-detector/start",
                   by_api_handler(&APIHandler::on_beat_detector_start));

  router->http_get("/api/beat-detector/stop",
                   by_api_handler(&APIHandler::on_beat_detector_stop));

  router->http_get("/api/beat-detector/status",
                   by_api_handler(&APIHandler::on_beat_detector_status));

  router->http_get("/api/tempo", by_api_handler(&APIHandler::on_tempo));

  router->http_post("/api/update-program",
                    by_api_handler(&APIHandler::on_update_program));

  router->http_get("/api/log", by_api_handler(&APIHandler::on_log));

  // GET request to homepage.
  router->http_get(R"(/:path(.*)\.:ext(.*))",
                   restinio::path2regex::options_t{}.strict(true),
                   by_file_handler(&FileHandler::on_file_request));

  // GET request to homepage.
  router->http_get("/", by_file_handler(&FileHandler::on_root_request));

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

HTTPServer::HTTPServer(const http_server_parameters_t &http_server_parameters,
                       StateManager &state_manager, Logger &logger,
                       beat_detector::BeatDetector &beat_detector)
    : state_manager_{state_manager}, logger_{logger},
      beat_detector_{beat_detector}, certs_dir_{
                                         http_server_parameters.certs_dir} {

  using std::literals::chrono_literals::operator""s;

  SPDLOG_INFO("Starting HTTP server, listening on: {}:{}",
              http_server_parameters.address, http_server_parameters.port);

  using traits_t = restinio::tls_traits_t<restinio::asio_timer_manager_t,
                                          //  restinio::shared_ostream_logger_t,
                                          restinio::null_logger_t, router_t>;

  // Since RESTinio supports both stand-alone ASIO and boost::ASIO
  // we specify an alias for a concrete asio namesace.
  // That's makes it possible to compile the code in both cases.
  // Typicaly only one of ASIO variants would be used,
  // and so only asio::* or only boost::asio::* would be applied.
  namespace asio_ns = restinio::asio_ns;

  const std::filesystem::path certs_dir =
      std::filesystem::path(http_server_parameters.certs_dir);

  asio_ns::ssl::context tls_context{asio_ns::ssl::context::sslv23};
  tls_context.set_options(asio_ns::ssl::context::default_workarounds |
                          asio_ns::ssl::context::no_sslv2 |
                          asio_ns::ssl::context::single_dh_use);
  std::vector<std::filesystem::path> certificate_paths{
      certificate_file_path(), key_file_path(), dh_params_file_path()};

  for (std::filesystem::path &cert_path : certificate_paths) {
    if (!std::filesystem::exists(cert_path)) {
      SPDLOG_ERROR("Missing certificate file {}. Couldn't start HTTP server",
                   cert_path);
      return;
    }
  }

  tls_context.use_certificate_chain_file(certificate_file_path());
  tls_context.use_private_key_file(key_file_path(), asio_ns::ssl::context::pem);

  tls_context.use_tmp_dh_file(dh_params_file_path());

  restinio::run(
      restinio::on_this_thread<traits_t>()
          .address(http_server_parameters.address)
          .port(http_server_parameters.port)
          .request_handler(server_handler(http_server_parameters.root_dir))
          .read_next_http_message_timelimit(10s)
          .write_http_response_timelimit(1s)
          .handle_request_timeout(1s)
          .tls_context(std::move(tls_context)));
}
