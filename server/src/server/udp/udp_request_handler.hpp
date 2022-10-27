#ifndef UDP__REQUEST_HANDLER_HPP
#define UDP__REQUEST_HANDLER_HPP

#include <asio.hpp>
#include <string>

namespace server {

class UDPRequestHandler {
public:
  UDPRequestHandler(std::string_view request_body,
                    const asio::ip::udp::endpoint &remote_endpoint);

  std::string get_response();

private:
  std::string tempo();
  std::string time_response();

  std::string_view request_body_;
  const asio::ip::udp::endpoint &remote_endpoint_;
};
} // namespace server

#endif // UDP__REQUEST_HANDLER_HPP
