#ifndef SERVER__APPLICATION__HPP
#define SERVER__APPLICATION__HPP

#include <memory>
#include <thread>

#include "core/config.hpp"
#include "core/interfaces/service_manager.hpp"
#include "core/state_manager.hpp"
#include "logger/logger.hpp"
#include "server/server.hpp"
#include "server/server_parameters.hpp"

class BeatledApplication : public ServiceManagerInterface {
public:
  BeatledApplication(const BeatledConfig &beatled_config);

  void run();

  StateManager &state_manager() override { return state_manager_; }

private:
  void initialize_io_context();
  void start_threads();

  const server::server_parameters_t server_parameters_;
  StateManager state_manager_;
  server::Logger logger_;
  std::thread beat_detector_thread_;

  /// The io_context used to perform asynchronous operations.
  asio::io_context io_context_;

  /// The signal_set is used to register for process termination notifications.
  asio::signal_set signals_;
};

#endif // SERVER__APPLICATION__HPP
