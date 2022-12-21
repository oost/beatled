#ifndef SERVER__APPLICATION__HPP
#define SERVER__APPLICATION__HPP

#include <memory>
#include <thread>

#include "beat_detector/beat_detector.hpp"
#include "core/config.hpp"
#include "core/interfaces/service_manager.hpp"
#include "core/state_manager.hpp"
#include "http_server/http_server.hpp"
#include "logger/logger.hpp"
#include "server/server.hpp"
#include "server/server_parameters.hpp"
#include "tempo_broadcaster/tempo_broadcaster.hpp"
#include "udp_server/udp_server.hpp"

class BeatledApplication : public ServiceManagerInterface {
public:
  BeatledApplication(const BeatledConfig &beatled_config);

  void run();

  server::Server &server() { return server_; }

  ServiceControllerInterface &beat_detector() override {
    return beat_detector_;
  }
  ServiceControllerInterface &http_server() override { return http_server_; }
  ServiceControllerInterface &udp_server() override { return udp_server_; }
  ServiceControllerInterface &temp_broadcaster() override {
    return tempo_broadcaster_;
  }
  StateManager &state_manager() override { return state_manager_; }

private:
  const server::server_parameters_t server_parameters_;

  StateManager state_manager_;

  server::Server server_;
  server::UDPServer udp_server_;
  beat_detector::BeatDetector beat_detector_;
  server::TempoBroadcaster tempo_broadcaster_;
  server::HTTPServer http_server_;

  server::Logger logger_;
  std::thread beat_detector_thread_;
};

#endif // SERVER__APPLICATION__HPP