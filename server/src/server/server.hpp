//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H

#include <asio.hpp>
#include <memory>
#include <string>
#include <vector>

#include "server_parameters.hpp"

namespace server {
/// The top-level class of the HTTP server.
class Server {
public:
  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit Server(std::size_t thread_pool_size,
                  const http_server_parameters_t &http_server_parameters,
                  const udp_server_parameters_t &udp_server_parameters);

  Server &operator=(const Server &) = delete;

  /// Run the server's io_context loop.
  void run();

private:
  /// The number of threads that will call io_context::run().
  std::size_t thread_pool_size_;

  /// The io_context used to perform asynchronous operations.
  asio::io_context io_context_;

  /// The signal_set is used to register for process termination notifications.
  asio::signal_set signals_;

  http_server_parameters_t http_server_parameters_;
  udp_server_parameters_t udp_server_parameters_;
};
} // namespace server

#endif // SERVER_SERVER_H
