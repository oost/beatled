//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <chrono>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <vector>

#include <fmt/format.h>

#include "core/config.hpp"
#include "core/interfaces/service_manager.hpp"
#include "http_server/http_server.hpp"
#include "server/server.hpp"
#include "tempo_broadcaster/tempo_broadcaster.hpp"
#include "udp_server/udp_server.hpp"

namespace beatled::server {

using beatled::core::StateManager;

Server::Server(const parameters_t &server_parameters)
    : server_parameters_{server_parameters}, signals_{io_context_} {

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

    io_context_.stop();
  });
}

void Server::run() {
  SPDLOG_INFO("Running server");
  std::mutex exception_mtx;
  std::exception_ptr exception_caught;

  // Create a pool of threads to run all of the io_contexts.
  std::vector<std::shared_ptr<asio::thread>> threads;
  SPDLOG_INFO("Starting {} network worker threads", server_parameters_.thread_pool_size);

  for (std::size_t i = 0; i < server_parameters_.thread_pool_size; ++i) {
    auto thread = std::make_shared<asio::thread>([this, &exception_mtx, &exception_caught]() {
      try {
        io_context_.run();
      } catch (...) {
        std::lock_guard<std::mutex> lock(exception_mtx);
        if (!exception_caught) {
          exception_caught = std::current_exception();
        }
        io_context_.stop();
      }
    });
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

Server::parameters_t make_server_parameters(const core::Config &config) {
  BroadcastMode mode = BroadcastMode::Unicast;
  const std::string &broadcast_mode = config.broadcast_mode();
  if (broadcast_mode == "limited") {
    mode = BroadcastMode::Limited;
  } else if (broadcast_mode == "subnet") {
    mode = BroadcastMode::Subnet;
  } else if (broadcast_mode == "unicast") {
    mode = BroadcastMode::Unicast;
  } else {
    throw std::runtime_error{fmt::format(
        "Invalid --broadcast-mode '{}' (must be limited, subnet, or unicast)", broadcast_mode)};
  }

  return Server::parameters_t{
      .start_http_server = config.start_http_server(),
      .start_udp_server = config.start_udp_server(),
      .start_broadcaster = config.start_broadcaster(),
      .http =
          {
              config.address(),          // address
              config.http_port(),        // port
              config.root_dir(),         // root_dir
              config.certs_dir(),        // cert_dir
              config.cors_origin(),      // cors_origin
              config.api_token(),        // api_token
              config.no_tls(),           // no_tls
              config.qos_skew_warn_us(), // qos_skew_warn_us
              config.qos_skew_fail_us(), // qos_skew_fail_us
          },
      .udp = {config.udp_port()},
      .broadcasting = {config.broadcasting_address(), config.broadcasting_port(), mode},
      .logger = {20, config.log_level()},
      .thread_pool_size = config.pool_size(),
      .program_refresh_ms = config.program_refresh_ms(),
      .status_probe_ms = config.status_probe_ms(),
      .qos_skew_warn_us = config.qos_skew_warn_us(),
      .qos_skew_fail_us = config.qos_skew_fail_us(),
  };
}

} // namespace beatled::server