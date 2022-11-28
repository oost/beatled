add_library(beatled_tcp_server 
  tcp_server.cpp 
  tcp_session.cpp
)

target_link_libraries(beatled_tcp_server PRIVATE 
  asio asio::asio         
  beatled_protocol
  beatled_udp
  beatled_state_manager
)

target_include_directories(beatled_tcp_server PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

