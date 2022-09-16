add_subdirectory(http)
add_subdirectory(udp)
add_subdirectory(tempo_broadcaster)
add_subdirectory(logger)

add_library(server server.cpp)
target_link_libraries(server PRIVATE asio asio::asio http_server udp_server tempo_broadcaster logger)

