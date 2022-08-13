add_library(http_server http_server.cpp)
target_link_libraries(http_server asio restinio::restinio nlohmann_json)
