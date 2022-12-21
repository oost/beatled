#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <filesystem>
#include <memory>
#include <string>

#include <asio.hpp>
#include <restinio/all.hpp>
#include <restinio/tls.hpp>

#include "./server_parameters.hpp"
#include "beat_detector/beat_detector.hpp"
#include "core/interfaces/service_controller.hpp"
#include "core/interfaces/service_manager.hpp"
#include "core/state_manager.hpp"
#include "logger/logger.hpp"

namespace server {

class HTTPServer : public ServiceControllerInterface {
public:
  using router_t = restinio::router::express_router_t<>;

  using traits_t = restinio::tls_traits_t<restinio::asio_timer_manager_t,
                                          //  restinio::shared_ostream_logger_t,
                                          restinio::null_logger_t, router_t>;
  using http_server_t = restinio::http_server_t<traits_t>;
  using settings_t = restinio::server_settings_t<traits_t>;

  HTTPServer(const std::string &id,
             const http_server_parameters_t &http_server_parameters,
             ServiceManagerInterface &service_manager,
             asio::io_context &io_context, Logger &logger);

  void start_sync() override;
  void stop_sync() override;

private:
  const char *SERVICE_NAME = "HTTP Server";
  const char *service_name() const override { return SERVICE_NAME; }

  std::filesystem::path certificate_file_path() {
    return certs_dir_ / "cert.pem";
  }
  std::filesystem::path key_file_path() { return certs_dir_ / "key.pem"; }
  std::filesystem::path dh_params_file_path() {
    return certs_dir_ / "dh_param.pem";
  }

  asio::io_context &io_context_;
  Logger &logger_;
  ServiceManagerInterface &service_manager_;
  std::filesystem::path certs_dir_;

  std::unique_ptr<http_server_t> restinio_server_;
  auto server_handler(const std::string &root_dir);
};
} // namespace server
#endif // HTTP_SERVER_H