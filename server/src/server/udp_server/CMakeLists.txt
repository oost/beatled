add_library(beatled_udp_server 
  udp_server.cpp 
  udp_request_handler.cpp
)

target_link_libraries(beatled_udp_server PRIVATE 
  asio asio::asio         
  beatled_protocol
  beatled_udp
  beatled_state_manager
)

