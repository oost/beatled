#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <asio.hpp>
#include <memory>
#include <string>

#include "beat_detector/beat_detector.hpp"
#include "logger/logger.hpp"
#include "server_parameters.hpp"
#include "state_manager/state_manager.hpp"

namespace server {

class HTTPServer {
public:
  HTTPServer(asio::io_context &io_context,
             const http_server_parameters_t &http_server_parameters,
             StateManager &state_manager, Logger &logger,
             beat_detector::BeatDetector &beat_detector);

private:
  StateManager &state_manager_;
  Logger &logger_;
  beat_detector::BeatDetector &beat_detector_;

  auto server_handler(const std::string &root_dir);
};
} // namespace server
#endif // HTTP_SERVER_H