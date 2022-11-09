#ifndef UDP__REQUEST_HANDLER_HPP
#define UDP__REQUEST_HANDLER_HPP

#include <asio.hpp>
#include <vector>

#include "udp_buffer.hpp"

namespace server {

class UDPRequestHandler {
public:
  UDPRequestHandler(UDPRequestBuffer::Ptr request_buffer_ptr);
  UDPResponseBuffer::Ptr response();

private:
  const UDPRequestBuffer::Ptr request_buffer_ptr_;

  template <typename T>
  UDPResponseBuffer::Ptr make_response_buffer(const T &data);

  UDPResponseBuffer::Ptr process_tempo_request();
  UDPResponseBuffer::Ptr process_time_request();
  UDPResponseBuffer::Ptr process_hello_request();
  UDPResponseBuffer::Ptr error_response(uint8_t error_code);
};
} // namespace server

#endif // UDP__REQUEST_HANDLER_HPP
