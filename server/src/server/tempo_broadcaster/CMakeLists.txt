add_library(tempo_broadcaster tempo_broadcaster.cpp broadcast_loop.cpp)
target_link_libraries(tempo_broadcaster PRIVATE asio asio::asio )