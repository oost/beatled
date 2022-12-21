add_library(beatled_http_server STATIC 
  http_server.cpp
  api_handler.cpp 
  file_handler.cpp
)

target_link_libraries(beatled_http_server PUBLIC 
  asio asio::asio 
  restinio::restinio 
  nlohmann_json::nlohmann_json
  beatled_logger
  beatled_core
  beat_detector
  OpenSSL::SSL
  OpenSSL::Crypto
)

target_include_directories(beatled_http_server PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)