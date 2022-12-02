cmake_minimum_required(VERSION 3.18)

message(Compiling if_test)
variable_watch(VCPKG_TARGET_TRIPLET)

include(/tmp/vcpkg/triplets/community/arm64-linux.cmake)

project(test_cmake_triplet)

set (CMAKE_CXX_STANDARD 20)

find_package(spdlog CONFIG REQUIRED)

add_executable(if_test main.cpp)

target_link_libraries(if_test spdlog::spdlog)

install(TARGETS if_test DESTINATION bin)

