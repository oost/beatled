#ifndef UDP__REQUEST_HANDLER_HPP
#define UDP__REQUEST_HANDLER_HPP

#include <asio.hpp>
#include <vector>

#include "state_manager/state_manager.hpp"
#include "udp/udp_buffer.hpp"

namespace server {

class UDPRequestHandler {
public:
  UDPRequestHandler(UDPRequestBuffer *request_buffer_ptr,
                    StateManager &state_manager);
  ResponseBuffer::Ptr response();

private:
  ResponseBuffer::Ptr process_tempo_request();
  ResponseBuffer::Ptr process_time_request();
  ResponseBuffer::Ptr process_hello_request();
  ResponseBuffer::Ptr error_response(uint8_t error_code);

  UDPRequestBuffer *request_buffer_ptr_;
  StateManager &state_manager_;
};
} // namespace server

#endif // UDP__REQUEST_HANDLER_HPP
