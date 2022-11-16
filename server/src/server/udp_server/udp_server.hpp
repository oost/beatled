#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <asio.hpp>
#include <memory>
#include <string>

#include "server_parameters.hpp"
#include "state_manager/state_manager.hpp"

namespace server {

class UDPServer {
public:
  UDPServer(asio::io_context &io_context,
            const udp_server_parameters_t &server_parameters,
            StateManager::Ptr state_manager);

private:
  void do_receive();

  void send_response(const std::string &sendBuf,
                     const asio::ip::udp::endpoint &remote_endpoint);

  asio::ip::udp::socket socket_;

  // asio::ip::udp::endpoint remote_endpoint_;
  StateManager::Ptr state_manager_;
};
} // namespace server
#endif // UDP_SERVER_H
