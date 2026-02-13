#include <memory>
#include <vector>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "./application.hpp"
#include "beat_detector/beat_detector.hpp"
#include "config.hpp"
#include "http_server/http_server.hpp"
#include "tempo_broadcaster/tempo_broadcaster.hpp"
#include "udp_server/udp_server.hpp"

const char *BEAT_DETECTOR_ID = "beat-detector";
const char *HTTP_SERVER_ID = "http-server";
const char *UDP_SERVER_ID = "udp-server";
const char *TEMPO_BROADCASTER_ID = "tempo-broadcaster";

using namespace beatled;
using beatled::core::Config;

Application::Application(const Config &beatled_config)
    : server_parameters_{beatled_config.server_parameters()},
      signals_{io_context_}, logger_{server_parameters_.logger} {

  std::unique_ptr<server::TempoBroadcaster> tempo_broadcaster =
      std::make_unique<server::TempoBroadcaster>(
          TEMPO_BROADCASTER_ID, io_context_,
          std::chrono::seconds(2), std::chrono::seconds(2),
          server_parameters_.broadcasting, state_manager_);

  registerController(std::make_unique<beatled::detector::BeatDetector>(
      BEAT_DETECTOR_ID, 44100, beatled::constants::audio_buffer_size,
      [&, tp = tempo_broadcaster.get()](uint64_t beat_time_ref, double tempo,
                                        double estimated_tempo,
                                        uint32_t beat_count) {
        uint64_t next_beat_time_ref = state_manager_.get_next_beat_time_ref();

        SPDLOG_INFO("Beat {}, {} vs. prediction {}", beat_time_ref, tempo,
                    static_cast<int64_t>(beat_time_ref) -
                        static_cast<int64_t>(next_beat_time_ref));
      },
      [&, tp = tempo_broadcaster.get()](uint64_t next_beat_time_ref,
                                        double tempo, double estimated_tempo,
                                        uint32_t beat_count) {
        SPDLOG_INFO("Next beat {}, {}", next_beat_time_ref, tempo);
        state_manager_.update_tempo(tempo, next_beat_time_ref);
        state_manager_.update_next_beat(next_beat_time_ref);

        tp->broadcast_next_beat(next_beat_time_ref, beat_count);
      }));

  registerController(std::make_unique<server::UDPServer>(
      UDP_SERVER_ID, io_context_, server_parameters_.udp, state_manager_));

  registerController(std::make_unique<server::HTTPServer>(
      HTTP_SERVER_ID, server_parameters_.http, *this, io_context_, logger_));

  registerController(std::move(tempo_broadcaster));

  initialize_io_context();
}

void Application::run() {
  if (server_parameters_.start_udp_server) {
    service(UDP_SERVER_ID)->start();
  }

  if (server_parameters_.start_broadcaster) {
    service(TEMPO_BROADCASTER_ID)->start();
  }

  if (server_parameters_.start_http_server) {
    service(HTTP_SERVER_ID)->start();
  }

  start_threads();
  SPDLOG_INFO("Stopped servers. Waiting for beat detection thread.");
  service(BEAT_DETECTOR_ID)->stop();
}

void Application::initialize_io_context() {
  SPDLOG_INFO("Initializing server");
  // Register to handle the signals that indicate when the server should
  // exit. It is safe to register for the same signal multiple times in a
  // program, provided all registration for the specified signal is made
  // through Asio.
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)
  signals_.async_wait([this](std::error_code /*ec*/, int /*signo*/) {
    // The server is stopped by cancelling all outstanding asynchronous
    // operations. Once all operations have finished the io_context::run()
    // call will exit.
    SPDLOG_INFO("Stopping server on SIGINT signal");
    for (auto &&[first, second] : services()) {
      second->stop();
    }
    io_context_.stop();
  });
}

void Application::start_threads() {
  SPDLOG_INFO("Running server");
  std::exception_ptr exception_caught;

  // Create a pool of threads to run all of the io_contexts.
  std::vector<std::shared_ptr<asio::thread>> threads;
  SPDLOG_INFO("Starting {} network worker threads",
              server_parameters_.thread_pool_size);

  for (std::size_t i = 0; i < server_parameters_.thread_pool_size; ++i) {
    std::shared_ptr<asio::thread> thread(
        new asio::thread([this, &exception_caught]() {
          try {
            io_context_.run();
          } catch (...) {
            exception_caught = std::current_exception();
          }
        }));
    threads.push_back(thread);
  }

  SPDLOG_INFO("Waiting for threads to join");
  // Wait for all threads in the pool to exit.
  for (std::size_t i = 0; i < threads.size(); ++i)
    threads[i]->join();

  // If an error was detected it should be propagated.
  if (exception_caught)
    std::rethrow_exception(exception_caught);
}