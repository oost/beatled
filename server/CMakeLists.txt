cmake_minimum_required(VERSION 3.10)
project(RPIZBeatServer)


add_executable(test_hello test.cpp)

# # add the executable
# add_executable(test1 test1.cpp)

add_subdirectory(lib)

add_executable(test_restinio test_restinio.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(test_restinio PRIVATE Threads::Threads)

# # Make your project dependent on restinio,
# # and let cmake deal with all the headers paths and linked libs.
target_link_libraries(test_restinio PRIVATE restinio::restinio)

# find_path(CLARA_INCLUDE_DIRS "clara.hpp")
target_include_directories(test_restinio PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lib/clara)
target_include_directories(test_restinio PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lib/json)

# target_include_directories(test_restinio PRIVATE ${CMAKE_CURRENT_LIST_DIR}/rapidjson/include)
# target_compile_definitions(test_restinio PRIVATE -DRAPIDJSON_HAS_STDSTRING -DRAPIDJSON_HAS_CXX11_RVALUE_REFS)


SET(BINARY_INSTALL_DIR ${PROJECT_BINARY_DIR}/target)

SET(INSTALL_TARGETS test_restinio test_hello)

install(
  TARGETS ${INSTALL_TARGETS}
  DESTINATION ${BINARY_INSTALL_DIR}
)

