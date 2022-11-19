add_library(beatled_config config.cpp)

target_include_directories(beatled_config PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(beatled_config PUBLIC 
  fmt::fmt
  bfg::lyra 
  beatled_server
)
