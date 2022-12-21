#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <asio.hpp>
#include <memory>
#include <string>

#include "core/interfaces/service_controller.hpp"
#include "core/state_manager.hpp"
#include "server_parameters.hpp"

namespace server {

class UDPServer : public ServiceControllerInterface {
public:
  UDPServer(asio::io_context &io_context,
            const udp_server_parameters_t &server_parameters,
            StateManager &state_manager);

  void start_sync() override;
  void stop_sync() override;

private:
  void do_receive();

  asio::ip::udp::socket socket_;

  StateManager &state_manager_;
};
} // namespace server
#endif // UDP_SERVER_H
