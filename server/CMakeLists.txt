cmake_minimum_required(VERSION 3.10)
project(RPIZBeatServer)

add_executable(hello_world hello_world.cpp)

add_subdirectory(lib)

add_subdirectory(http)


set(BEAT_SERVER beat_server)
add_executable(${BEAT_SERVER} main.cpp)

target_link_libraries(${BEAT_SERVER} PRIVATE http_handler)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${BEAT_SERVER} PRIVATE Threads::Threads)

# # Make your project dependent on restinio,
# # and let cmake deal with all the headers paths and linked libs.
# find_path(CLARA_INCLUDE_DIRS "clara.hpp")

target_include_directories(${BEAT_SERVER} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lib/clara)
target_link_libraries(${BEAT_SERVER} PRIVATE fmt::fmt-header-only)

# target_include_directories(test_restinio PRIVATE ${CMAKE_CURRENT_LIST_DIR}/rapidjson/include)
# target_compile_definitions(test_restinio PRIVATE -DRAPIDJSON_HAS_STDSTRING -DRAPIDJSON_HAS_CXX11_RVALUE_REFS)


SET(BINARY_INSTALL_DIR ${PROJECT_BINARY_DIR}/target)

SET(INSTALL_TARGETS ${BEAT_SERVER} hello_world)

install(
  TARGETS ${INSTALL_TARGETS}
  DESTINATION ${BINARY_INSTALL_DIR}
)

