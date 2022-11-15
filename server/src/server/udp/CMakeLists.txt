add_library(udp_server 
  udp_server.cpp 
  udp_request_handler.cpp
  udp_buffer.cpp 
  udp_response_buffer.cpp 
)
target_link_libraries(udp_server PRIVATE 
  asio asio::asio         
  beatled_protocol
)


