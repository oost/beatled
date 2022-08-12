
cmake_minimum_required(VERSION 3.10)

# if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
#   set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
#       CACHE STRING "")
#   message("Using VCPKG")
# else(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
#   message("VCPKG NOT FOUND")
# endif()

# message("VCPKG=" $ENV{VCPKG_ROOT} " CMAKE_TOOLCHAIN_FILE=" ${CMAKE_TOOLCHAIN_FILE})

# set the project name

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}../lib/restinio-0.6.16/dev")
# message("CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
project(RPIZBeatServer)


add_executable(test_hello test.cpp)

# # add the executable
# add_executable(test1 test1.cpp)

# set(RESTINIO_FIND_DEPS=ON)
# set(CMAKE_INSTALL_PREFIX=/app/external/restinio-0.6.16/dev)
# set(CMAKE_PREFIX_PATH=/app/external/restinio-0.6.16/dev)

add_executable(test_restinio test_restinio.cpp)

find_package(restinio REQUIRED CONFIG)
find_package(unofficial-http-parser REQUIRED)
find_package(fmt REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(test_restinio PRIVATE Threads::Threads)

# # # Make your project dependent on restinio,
# # # and let cmake deal with all the headers paths and linked libs.
target_link_libraries(test_restinio PRIVATE restinio::restinio)

find_path(CLARA_INCLUDE_DIRS "clara.hpp")
target_include_directories(test_restinio PRIVATE ${CLARA_INCLUDE_DIRS})