#ifndef UDP__REQUEST_HANDLER_HPP
#define UDP__REQUEST_HANDLER_HPP

#include <asio.hpp>
#include <vector>

#include "core/state_manager.hpp"
#include "udp/udp_buffer.hpp"

using beatled::core::StateManager;

namespace beatled::server {

class UDPRequestHandler {
public:
  UDPRequestHandler(UDPRequestBuffer *request_buffer_ptr,
                    StateManager &state_manager);
  DataBuffer::Ptr response();

private:
  DataBuffer::Ptr process_tempo_request();
  DataBuffer::Ptr process_time_request();
  DataBuffer::Ptr process_hello_request();
  DataBuffer::Ptr error_response(uint8_t error_code);

  UDPRequestBuffer *request_buffer_ptr_;
  StateManager &state_manager_;
};
} // namespace beatled::server

#endif // UDP__REQUEST_HANDLER_HPP
