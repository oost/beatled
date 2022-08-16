#include <chrono>
#include <iostream>
#include <sys/time.h>

#include "../state_manager/state_manager.hpp"
#include "beat_detector.hpp"

BeatDetector::BeatDetector()
    : signals_(io_context_), timer_(asio::high_resolution_timer(
                                 io_context_, std::chrono::seconds(3))) {

  // Register to handle the signals that indicate when the server should exit.
  // It is safe to register for the same signal multiple times in a program,
  // provided all registration for the specified signal is made through Asio.
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)
  signals_.async_wait([this](std::error_code /*ec*/, int /*signo*/) {
    // The server is stopped by cancelling all outstanding asynchronous
    // operations. Once all operations have finished the io_context::run()
    // call will exit.
    io_context_.stop();
  });
}

void BeatDetector::run() {

  std::cout << "Starting Beat Detector" << std::endl;
  // post(io_context_, [this]() { do_detect_tempo(); });
  // io_context_.run();
}

// void BeatDetector::do_detect_tempo() {
//   std::cout << "Detecting tempo" << std::endl;

//   timespec ts;
//   clock_gettime(CLOCK_REALTIME, &ts);
//   precise_time_t timeref = {ts.tv_sec, ts.tv_nsec};
//   float tempo = 124.5;
//   std::cout << "Detected tempo" << tempo << " and timeref " << timeref.tv_sec
//             << std::endl;

//   // StateManager::get_instance().->update_tempo(tempo, timeref);

//   timer_.expires_at(std::chrono::high_resolution_clock::now() +
//                     std::chrono::seconds(3));
//   timer_.async_wait([this](auto) { do_detect_tempo(); });
// }
