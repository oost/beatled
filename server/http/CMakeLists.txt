add_library(http_handler server_handler.cpp)
target_link_libraries(http_handler PRIVATE restinio::restinio)
target_include_directories(http_handler PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../lib/json)
