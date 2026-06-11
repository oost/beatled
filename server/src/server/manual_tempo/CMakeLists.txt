
add_library(beatled_manual_tempo
  manual_tempo.cpp
)

target_link_libraries(beatled_manual_tempo
  PRIVATE
    asio
    asio::asio
  PUBLIC
    beatled_core
)

target_include_directories(beatled_manual_tempo PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
