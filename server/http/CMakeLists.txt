add_library(http_handler server_handler.cpp)
target_link_libraries(http_handler PRIVATE restinio::restinio json)
