add_library(udp_server udp_server.cpp udp_request_handler.cpp)
target_link_libraries(udp_server PRIVATE 
  asio asio::asio         
  beatled_protocol
)


