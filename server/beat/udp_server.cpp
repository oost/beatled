#include <cstdlib>
#include <iostream>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <asio/signal_set.hpp>

using asio::ip::udp;

enum
{
  max_length = 1024
};

void server(asio::io_context &io_context, unsigned short port)
{
  std::cout << "Starting UDP server. Listening on " << port << "\n";

  udp::socket sock(io_context, udp::endpoint(udp::v4(), port));
  for (;;)
  {
    char data[max_length];
    udp::endpoint sender_endpoint;
    size_t length = sock.receive_from(
        asio::buffer(data, max_length), sender_endpoint);

    std::cout << "Received: " << data << "\n";

    sock.send_to(asio::buffer(data, length), sender_endpoint);
  }
}

void start_udp_server(unsigned short port)
{
  asio::io_context io_context;
  asio::signal_set signals(io_context, SIGINT, SIGTERM);
  signals.async_wait([&](auto, auto)
                     { io_context.stop(); });
  server(io_context, port);
}