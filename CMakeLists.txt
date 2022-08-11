
cmake_minimum_required(VERSION 3.10)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
  message("Using VCPKG")
else(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message("VCPKG NOT FOUND")
endif()

message("VCPKG=" $ENV{VCPKG_ROOT} " CMAKE_TOOLCHAIN_FILE=" ${CMAKE_TOOLCHAIN_FILE})

# set the project name
project(RPIZBeatServer)


# add the executable
# add_executable(test1 test1.cpp)

add_executable(test_restinio test_restinio.cpp)

# RESTinio dependencies:
# 1. ASIO or Boost::ASIO (goes as headers, vcpkg knows where)
# 2. HTTP parser
find_package(unofficial-http-parser REQUIRED)
# 3. fmtlib
find_package(fmt REQUIRED)
# RESTinio itself
find_package(restinio REQUIRED)

# Make your project dependent on restinio,
# and let cmake deal with all the headers paths and linked libs.
target_link_libraries(test_restinio PRIVATE restinio::restinio)

find_path(CLARA_INCLUDE_DIRS "clara.hpp")
target_include_directories(test_restinio PRIVATE ${CLARA_INCLUDE_DIRS})