#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <filesystem>
#include <memory>
#include <string>

#include <asio.hpp>

#include "./server_parameters.hpp"
#include "beat_detector/beat_detector.hpp"
#include "logger/logger.hpp"
#include "state_manager/state_manager.hpp"

namespace server {

class HTTPServer {
public:
  HTTPServer(const http_server_parameters_t &http_server_parameters,
             StateManager &state_manager, Logger &logger,
             beat_detector::BeatDetector &beat_detector);

private:
  std::filesystem::path certificate_file_path() {
    return certs_dir_ / "cert.pem";
  }
  std::filesystem::path key_file_path() { return certs_dir_ / "key.pem"; }
  std::filesystem::path dh_params_file_path() {
    return certs_dir_ / "dh_param.pem";
  }

  StateManager &state_manager_;
  Logger &logger_;
  beat_detector::BeatDetector &beat_detector_;

  std::filesystem::path certs_dir_;

  auto server_handler(const std::string &root_dir);
};
} // namespace server
#endif // HTTP_SERVER_H