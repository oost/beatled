add_library(beatled_state_manager 
  state_manager.cpp
  client_status.cpp
)

target_link_libraries(beatled_state_manager PUBLIC
  asio::asio
  fmt::fmt 
  nlohmann_json::nlohmann_json
  beatled_protocol
)

target_include_directories(beatled_state_manager PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
