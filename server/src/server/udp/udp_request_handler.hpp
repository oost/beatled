#ifndef UDP__REQUEST_HANDLER_HPP
#define UDP__REQUEST_HANDLER_HPP

#include <asio.hpp>
#include <vector>

#include "udp_buffer.hpp"

namespace server {

class UDPRequestHandler {
public:
  UDPRequestHandler(UDPRequestBuffer::Ptr &&request_buffer_ptr);
  UDPResponseBuffer::Ptr response();

private:
  UDPRequestBuffer::Ptr request_buffer_ptr_;
  UDPResponseBuffer::Ptr response_buffer_ptr_;

  void process_tempo_request();
  void process_time_request();
  void process_hello_request();
  void error_response(uint8_t error_code);
};
} // namespace server

#endif // UDP__REQUEST_HANDLER_HPP
