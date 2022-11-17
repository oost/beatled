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

#include "logger/logger.hpp"
#include "server_parameters.hpp"
#include "state_manager/state_manager.hpp"

namespace server {
/// The top-level class of the HTTP server.
class Server {
public:
  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit Server(StateManager &state_manager,
                  const server_parameters_t &server_parameters);

  Server &operator=(const Server &) = delete;

  /// Run the server's io_context loop.
  void run();

private:
  server_parameters_t server_parameters_;

  /// The io_context used to perform asynchronous operations.
  asio::io_context io_context_;

  /// The signal_set is used to register for process termination notifications.
  asio::signal_set signals_;

  StateManager &state_manager_;
  Logger logger_;
};
} // namespace server

#endif // SERVER_SERVER_H
