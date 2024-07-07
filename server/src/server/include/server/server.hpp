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

#include "http_server/http_server.hpp"
#include "logger/logger.hpp"
#include "tempo_broadcaster/tempo_broadcaster.hpp"
#include "udp_server/udp_server.hpp"

namespace beatled {

class Application;

namespace server {
/// The top-level class of the HTTP server.
class Server {
public:
  using Ptr = std::unique_ptr<Server>;

  struct parameters_t {
    bool start_http_server;
    bool start_udp_server;
    bool start_broadcaster;
    HTTPServer::parameters_t http;
    UDPServer::parameters_t udp;
    TempoBroadcaster::parameters_t broadcasting;
    Logger::parameters_t logger;
    std::size_t thread_pool_size;
  };

  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit Server(const parameters_t &server_parameters);

  Server &operator=(const Server &) = delete;

  /// Run the server's io_context loop.
  void run();

  asio::io_context &io_context() { return io_context_; };

private:
  const parameters_t &server_parameters_;

  /// The io_context used to perform asynchronous operations.
  asio::io_context io_context_;

  /// The signal_set is used to register for process termination notifications.
  asio::signal_set signals_;
};
} // namespace server

} // namespace beatled

#endif // SERVER_SERVER_H
