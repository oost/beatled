//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <memory>
#include <vector>

#include "http/http_server.h"
#include "server.h"
#include "udp/udp_server.h"

using namespace server;

Server::Server(std::size_t thread_pool_size,
               const http_server_parameters_t &http_server_parameters,
               const udp_server_parameters_t &udp_server_parameters)
    : thread_pool_size_(thread_pool_size), signals_(io_context_),
      http_server_parameters_(http_server_parameters),
      udp_server_parameters_(udp_server_parameters) {
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

void Server::run() {
  // Create a pool of threads to run all of the io_contexts.
  std::vector<std::shared_ptr<asio::thread>> threads;
  for (std::size_t i = 0; i < thread_pool_size_; ++i) {
    std::shared_ptr<asio::thread> thread(
        new asio::thread([this]() { io_context_.run(); }));
    threads.push_back(thread);
  }

  UDPServer udp_server(io_context_, udp_server_parameters_);

  HTTPServer(io_context_, http_server_parameters_);

  std::cout << "Waiting for threads" << std::endl;

  // Wait for all threads in the pool to exit.
  for (std::size_t i = 0; i < threads.size(); ++i)
    threads[i]->join();
}

void Server::handle_stop() {}
