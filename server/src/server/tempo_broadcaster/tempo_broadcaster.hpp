#ifndef SERVER_TEMPO_BROADCASTER_H
#define SERVER_TEMPO_BROADCASTER_H

#include <asio.hpp>
#include <chrono>

#include "../../state_manager/state_manager.hpp"

class TempoBroadcaster {
public:
  TempoBroadcaster(asio::io_context &io_context,
                   std::chrono::nanoseconds alarm_period, short port);

private:
  void do_broadcast();

  asio::io_context &io_context_;

  asio::high_resolution_timer timer_;
  std::chrono::nanoseconds alarm_period_;

  asio::ip::udp::socket socket_;
  int count_;
  short port_;
};

#endif // SERVER_TEMPO_BROADCASTER_H