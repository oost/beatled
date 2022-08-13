add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_CURRENT_LIST_DIR}/asio/asio/include)

add_subdirectory(Lyra)
add_subdirectory(fmt)
add_subdirectory(json)