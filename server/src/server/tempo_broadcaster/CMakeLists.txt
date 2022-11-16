
add_library(beatled_tempo_broadcaster 
  tempo_broadcaster.cpp 
  broadcast_loop.cpp
)

target_link_libraries(beatled_tempo_broadcaster 
  PRIVATE 
    asio 
    asio::asio 
  PUBLIC 
    beatled_udp 
    beatled_state_manager
)

target_include_directories(beatled_tempo_broadcaster PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)