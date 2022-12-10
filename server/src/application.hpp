#ifndef SERVER__APPLICATION__HPP
#define SERVER__APPLICATION__HPP

#include <memory>
#include <thread>

#include "beat_detector/beat_detector.hpp"
#include "config/config.hpp"
#include "server/server.hpp"
#include "state_manager/state_manager.hpp"

class BeatledApplication {
public:
  BeatledApplication(const BeatledConfig &beatled_config);

  void start_beat_detector();
  void stop_beat_detector();

  void start();

private:
  const BeatledConfig &beatled_config_;
  StateManager state_manager_;
  beat_detector::BeatDetector beat_detector_;
  server::Server::Ptr server_;
  std::thread beat_detector_thread_;
};

#endif // SERVER__APPLICATION__HPP