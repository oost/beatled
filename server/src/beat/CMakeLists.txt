add_library(beat_worker beat_worker.cpp udp_server.cpp)
target_link_libraries(beat_worker asio)


