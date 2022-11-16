add_library(beatled_udp 
  udp_buffer.cpp 
  udp_response_buffer.cpp 
)

target_link_libraries(beatled_udp PRIVATE
  asio asio::asio         
  beatled_protocol
)

target_include_directories(beatled_udp PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
