#include "udp_server.h"
#include <chrono>
#include <iostream>
#include <thread>

#include "beat_worker.h"

namespace server {
void start_beat_worker() {
  using namespace std::chrono_literals;
  std::cout << "Starting beat worker" << std::endl;

  // start_udp_server(9090);

  while (1) {
    auto start = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(2000ms);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "Waited " << elapsed.count() << " ms\n";
  }
}

} // namespace server
