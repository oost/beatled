add_library(beatled_udp_server 
  udp_server.cpp 
  udp_request_handler.cpp
)

target_link_libraries(beatled_udp_server PRIVATE 
  asio asio::asio         
  beatled_protocol
  beatled_udp
  beatled_core
)

target_include_directories(beatled_udp_server PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

