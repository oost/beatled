#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <filesystem>
#include <memory>
#include <string>
#include <variant>

#include <asio.hpp>
#include <restinio/core.hpp>
#include <restinio/tls.hpp>

#include "beat_detector/beat_detector.hpp"
#include "core/interfaces/service_controller.hpp"
#include "core/interfaces/service_manager.hpp"
#include "core/state_manager.hpp"
#include "logger/logger.hpp"

namespace beatled::server {
using core::ServiceControllerInterface;
using core::ServiceManagerInterface;

// Build the TLS context used by the HTTPS listener: TLS 1.2+, cert chain,
// private key and DH params from the given PEM files. Throws
// std::runtime_error when a file is missing; OpenSSL errors for malformed
// contents propagate as asio system errors. Split out of the HTTPServer
// constructor so certificate loading is testable without a server.
restinio::asio_ns::ssl::context make_tls_context(const std::filesystem::path &cert_file,
                                                 const std::filesystem::path &key_file,
                                                 const std::filesystem::path &dh_params_file);

class HTTPServer : public ServiceControllerInterface {
public:
  using router_t = restinio::router::express_router_t<>;

  using tls_traits_t =
      restinio::tls_traits_t<restinio::asio_timer_manager_t, restinio::null_logger_t, router_t>;
  using plain_traits_t =
      restinio::traits_t<restinio::asio_timer_manager_t, restinio::null_logger_t, router_t>;

  using tls_server_t = restinio::http_server_t<tls_traits_t>;
  using plain_server_t = restinio::http_server_t<plain_traits_t>;

  using tls_settings_t = restinio::server_settings_t<tls_traits_t>;
  using plain_settings_t = restinio::server_settings_t<plain_traits_t>;

  struct parameters_t {
    const std::string address;
    std::uint16_t port;
    const std::string root_dir;
    const std::string certs_dir;
    std::string cors_origin;
    std::string api_token;
    bool no_tls{false};
    // QoS health-pip thresholds (microseconds) — surfaced through to
    // /api/qos via APIHandler::QosThresholds. Match the defaults in
    // core::Config so a never-overridden run stays consistent end-to-end.
    std::uint32_t qos_skew_warn_us{5000};
    std::uint32_t qos_skew_fail_us{20000};
  };

  HTTPServer(const std::string &id, const parameters_t &http_server_parameters,
             ServiceManagerInterface &service_manager, asio::io_context &io_context,
             Logger &logger);

  void start_sync() override;
  void stop_sync() override;

private:
  const char *SERVICE_NAME = "HTTP Server";
  const char *service_name() const override { return SERVICE_NAME; }

  std::filesystem::path certificate_file_path() { return certs_dir_ / "cert.pem"; }
  std::filesystem::path key_file_path() { return certs_dir_ / "key.pem"; }
  std::filesystem::path dh_params_file_path() { return certs_dir_ / "dh_param.pem"; }

  asio::io_context &io_context_;
  Logger &logger_;
  ServiceManagerInterface &service_manager_;
  std::filesystem::path certs_dir_;
  std::string cors_origin_;
  std::string api_token_;
  std::string address_;
  std::uint16_t port_;
  std::uint32_t qos_skew_warn_us_{5000};
  std::uint32_t qos_skew_fail_us_{20000};

  std::variant<std::unique_ptr<tls_server_t>, std::unique_ptr<plain_server_t>> server_;
  std::unique_ptr<router_t> server_handler(const std::string &root_dir);
};

} // namespace beatled::server
#endif // HTTP_SERVER_H