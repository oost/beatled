cmake_minimum_required(VERSION 3.18)

# Clan Tidy "-DCMAKE_CXX_CLANG_TIDY=/usr/bin/clang-tidy-3.9;-checks=*"
# CppCheck "-DCMAKE_CXX_CPPCHECK=/usr/bin/cppcheck;--std=c++11"
# IWYU "-DCMAKE_CXX_INCLUDE_WHAT_YOU_USE=/usr/bin/iwyu;--transitive_includes_only"
# LWYU cmake -DCMAKE_LINK_WHAT_YOU_USE=TRUE

project(RPIZBeatServer VERSION 0.1)

set (CMAKE_CXX_STANDARD 20)
set(BEAT_SERVER beat_server)
set(BUILD_SHARED_LIBS OFF)

option(BEATSERVER_ENABLE_TESTS "Enable tests" ON)
option(BUILD_DOCS "Build documentation" OFF)
option(SPDLOG_FMT_EXTERNAL_HO "Use external fmt header-only library instead of bundled" ON)
option(SPDLOG_FMT_EXTERNAL "Use external fmt library instead of bundled" ON) 

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(FFTW3 CONFIG REQUIRED)
# find_library(FFTW3::fftw3 fftw3)
# find_package(FFTW3l CONFIG REQUIRED)
# find_package(http-parser CONFIG REQUIRED)
find_package(lyra CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(restinio CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(date CONFIG REQUIRED)
find_package(portaudio CONFIG REQUIRED)
find_package(AudioFile CONFIG REQUIRED)
find_package(OpenSSL)
IF ( OPENSSL_FOUND )
  message("OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
  message("OpenSSL libraries: ${OPENSSL_LIBRARIES}")
ENDIF ( OPENSSL_FOUND )

add_subdirectory(external)
set(BEATLED_PROTOCOL_PATH "${CMAKE_CURRENT_LIST_DIR}/external/pico-w-beatled/lib/beatled_protocol")

include("${CMAKE_CURRENT_LIST_DIR}/cmake/beatled_protocol_import.cmake")
beatled_protocol_init()


add_subdirectory(src)

if (BEATSERVER_ENABLE_TESTS)
  message("Building tests")
  message("CMAKE_CROSSCOMPILING= ${CMAKE_CROSSCOMPILING}")
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

if (BUILD_DOCS)
  add_subdirectory(docs)
endif()