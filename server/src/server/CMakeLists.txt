add_subdirectory(http)
add_subdirectory(udp)

add_library(server server.cpp)
target_link_libraries(server asio http_server udp_server)

