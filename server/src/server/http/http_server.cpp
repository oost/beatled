#include <filesystem>
#include <iostream>
#include <map>
#include <memory>

#include <fmt/std.h>
#include <nlohmann/json.hpp>
#include <restinio/all.hpp>
#include <restinio/http_headers.hpp>
#include <restinio/tls.hpp>
#include <spdlog/spdlog.h>

#include "./api_handler.hpp"
#include "./file_handler.hpp"
#include "http_server/http_server.hpp"

using json = nlohmann::json;
namespace beatled::server {

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

std::unique_ptr<router_t>
HTTPServer::server_handler(const std::string &root_dir) {
  auto router = std::make_unique<router_t>();

  auto file_handler = std::make_shared<FileHandler>(root_dir);

  auto by_file_handler = [&](auto method) {
    using namespace std::placeholders;
    return std::bind(method, file_handler, _1, _2);
  };

  auto api_handler = std::make_shared<APIHandler>(service_manager_, logger_);
  auto by_api_handler = [&](auto method) {
    using namespace std::placeholders;
    return std::bind(method, api_handler, _1, _2);
  };

  router->http_get("/api/status", by_api_handler(&APIHandler::on_get_status));

  router->http_post("/api/service/control",
                    by_api_handler(&APIHandler::on_post_service_control));

  router->http_get("/api/tempo", by_api_handler(&APIHandler::on_get_tempo));

  router->http_post("/api/program",
                    by_api_handler(&APIHandler::on_post_program));

  router->http_get("/api/program", by_api_handler(&APIHandler::on_get_program));

  router->http_get("/api/log", by_api_handler(&APIHandler::on_get_log));

  // GET request to homepage.
  router->http_get(R"(/:path(.*)\.:ext(.*))",
                   restinio::path2regex::options_t{}.strict(true),
                   by_file_handler(&FileHandler::on_file_request));

  // GET request to homepage.
  router->http_get("/", by_file_handler(&FileHandler::on_root_request));

  router->http_head(R"(/api/:path(.*))",
                    by_api_handler(&APIHandler::on_preflight));

  router->add_handler(restinio::http_method_options(), R"(/api/:path(.*))",
                      by_api_handler(&APIHandler::on_preflight));

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

HTTPServer::HTTPServer(const std::string &id,
                       const parameters_t &http_server_parameters,
                       ServiceManagerInterface &service_manager,
                       asio::io_context &io_context, Logger &logger)
    : ServiceControllerInterface{id}, service_manager_{service_manager},
      io_context_{io_context}, logger_{logger},
      certs_dir_{http_server_parameters.certs_dir} {
  SPDLOG_INFO("Creating {}", name());

  using std::literals::chrono_literals::operator""s;

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

  SPDLOG_INFO("{}: listening on {}:{}", name(), http_server_parameters.address,
              http_server_parameters.port);
  restinio_server_ = std::make_unique<http_server_t>(
      restinio::external_io_context(io_context),
      settings_t{}
          .address(http_server_parameters.address)
          .port(http_server_parameters.port)
          .request_handler(server_handler(http_server_parameters.root_dir))
          .read_next_http_message_timelimit(10s)
          .write_http_response_timelimit(1s)
          .handle_request_timeout(1s)
          .tls_context(std::move(tls_context)));
}

void HTTPServer::start_sync() {
  asio::post(io_context_, [&] { restinio_server_->open_sync(); });
}

void HTTPServer::stop_sync() {
  asio::post(io_context_, [&] { restinio_server_->close_sync(); });
}

} // namespace beatled::server