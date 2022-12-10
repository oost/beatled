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
#include <iostream>
#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

#include "http_server/http_server.hpp"
#include "server/server.hpp"
#include "tempo_broadcaster/tempo_broadcaster.hpp"
#include "udp_server/udp_server.hpp"

using namespace server;

Server::Server(StateManager &state_manager,
               const server_parameters_t &server_parameters)
    : state_manager_{state_manager}, signals_{io_context_},
      server_parameters_{server_parameters}, logger_(server_parameters.logger) {

  logger_.log_message("INFO", "Initializing server");
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
  SPDLOG_INFO("Starting server");
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

  std::unique_ptr<UDPServer> udp_server;
  std::unique_ptr<TempoBroadcaster> tempo_broadcaster;
  std::unique_ptr<HTTPServer> http_server;

  if (server_parameters_.start_udp_server) {
    udp_server = std::make_unique<UDPServer>(
        io_context_, server_parameters_.udp, state_manager_);
  }
  if (server_parameters_.start_broadcaster) {
    tempo_broadcaster = std::make_unique<TempoBroadcaster>(
        io_context_, std::chrono::milliseconds((60 * 1000) / 120),
        std::chrono::seconds(2), server_parameters_.broadcasting,
        state_manager_);
  }
  if (server_parameters_.start_http_server) {
    http_server = std::make_unique<HTTPServer>(
        io_context_, server_parameters_.http, state_manager_, logger_);
  }

  SPDLOG_INFO("Waiting for threads to join");
  // Wait for all threads in the pool to exit.
  for (std::size_t i = 0; i < threads.size(); ++i)
    threads[i]->join();

  // If an error was detected it should be propagated.
  if (exception_caught)
    std::rethrow_exception(exception_caught);
}
