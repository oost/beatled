#ifndef UDP__REQUEST_HANDLER_HPP
#define UDP__REQUEST_HANDLER_HPP

#include <asio.hpp>
#include <vector>

#include "state_manager/state_manager.hpp"
#include "udp/udp_buffer.hpp"

namespace server {

class UDPRequestHandler {
public:
  UDPRequestHandler(UDPRequestBuffer::Ptr &&request_buffer_ptr,
                    StateManager &state_manager);
  UDPResponseBuffer::Ptr response();

private:
  UDPRequestBuffer::Ptr request_buffer_ptr_;
  StateManager &state_manager_;
  // UDPResponseBuffer::Ptr response_buffer_ptr_;

  UDPResponseBuffer::Ptr process_tempo_request();
  UDPResponseBuffer::Ptr process_time_request();
  UDPResponseBuffer::Ptr process_hello_request();
  UDPResponseBuffer::Ptr error_response(uint8_t error_code);
};
} // namespace server

#endif // UDP__REQUEST_HANDLER_HPP
