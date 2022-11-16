add_subdirectory(http)
add_subdirectory(udp)
add_subdirectory(udp_server)
add_subdirectory(tempo_broadcaster)
add_subdirectory(logger)

add_library(beatled_server server.cpp)

target_link_libraries(beatled_server PUBLIC 
  asio 
  asio::asio 
  beatled_http_server 
  beatled_udp_server 
  beatled_tempo_broadcaster 
  beatled_logger
)

