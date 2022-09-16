add_library(http_server STATIC http_server.cpp)
target_link_libraries(http_server PRIVATE asio asio::asio restinio::restinio nlohmann_json::nlohmann_json)
