cmake_minimum_required(VERSION 3.15)
project(restinio_project CXX)

# include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
# conan_basic_setup()

find_package(restinio)

add_executable(hello_world_minimal hello_world_minimal.cpp)


# Use the global target
target_link_libraries(hello_world_minimal restinio::restinio)
set_property(TARGET hello_world_minimal PROPERTY CXX_STANDARD 14)