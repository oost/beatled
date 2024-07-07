add_library(beatled_core 
  config.cpp
  state_manager.cpp
  client_status.cpp  
)

target_include_directories(beatled_core PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(beatled_core PUBLIC 
  fmt::fmt
  bfg::lyra 
  spdlog::spdlog 
  asio::asio
  nlohmann_json::nlohmann_json
  # beatled_server
  # beatled_protocol
)
