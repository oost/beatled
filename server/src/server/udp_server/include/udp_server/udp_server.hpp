#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <asio.hpp>
#include <memory>
#include <string>

#include "core/interfaces/service_controller.hpp"
#include "core/state_manager.hpp"

using beatled::core::ServiceControllerInterface;
using beatled::core::StateManager;

namespace beatled::server {

class UDPServer : public ServiceControllerInterface {
public:
  struct parameters_t {
    std::uint16_t port;
  };

  UDPServer(const std::string &id, asio::io_context &io_context,
            const parameters_t &server_parameters, StateManager &state_manager);

  void start_sync() override;
  void stop_sync() override;

private:
  const char *SERVICE_NAME = "UDP Server";
  const char *service_name() const override { return SERVICE_NAME; }

  void do_receive();

  asio::ip::udp::socket socket_;

  StateManager &state_manager_;
};
} // namespace beatled::server
#endif // UDP_SERVER_H
