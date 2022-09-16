add_library(udp_server udp_server.cpp)
target_link_libraries(udp_server PRIVATE asio asio::asio)


