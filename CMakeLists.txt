cmake_minimum_required(VERSION 3.13)

include(cmake/port.cmake)
include(cmake/hal_module.cmake)
include(cmake/utils.cmake)

project(pico_w_beatled C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
list(APPEND CMAKE_CXX_SOURCE_FILE_EXTENSIONS c)

port_init()


set(WIFI_SSID "$ENV{WIFI_SSID}" CACHE INTERNAL "WiFi SSID for examples")
set(WIFI_PASSWORD "$ENV{WIFI_PASSWORD}" CACHE INTERNAL "WiFi password for examples")
set(BEATLED_SERVER_NAME "$ENV{BEATLED_SERVER_NAME}")
if(POSIX_PORT AND "${BEATLED_SERVER_NAME}" STREQUAL "")
  set(BEATLED_SERVER_NAME "localhost")
endif()

message("Compiling with WIFI_SSID ${WIFI_SSID} and password ${WIFI_PASSWORD}, BEATLED_SERVER_NAME=${BEATLED_SERVER_NAME}")

# Generate generated/beatled_version.h on every build so the embedded
# git hash always matches whatever was just compiled.
set(BEATLED_VERSION_HEADER ${CMAKE_BINARY_DIR}/generated/beatled_version.h)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)
add_custom_target(beatled_version_header ALL
  COMMAND ${CMAKE_COMMAND}
    -DBEATLED_SRC_DIR=${CMAKE_CURRENT_LIST_DIR}
    -DOUT=${BEATLED_VERSION_HEADER}
    -P ${CMAKE_CURRENT_LIST_DIR}/cmake/gen_version.cmake
  BYPRODUCTS ${BEATLED_VERSION_HEADER}
  COMMENT "Generating beatled_version.h"
)
include_directories(${CMAKE_BINARY_DIR}/generated)

option(PICO_AUTOTEST "Run with auto tests" OFF)
option(PICO_PROBE "Debug with Pico probe" OFF)

if(NOT DEFINED NUM_PIXELS)
  message(FATAL_ERROR "NUM_PIXELS must be set. Example: cmake -DNUM_PIXELS=30 ..")
endif()
add_definitions(-DNUM_PIXELS=${NUM_PIXELS})
message("Compiling with NUM_PIXELS=${NUM_PIXELS}")

set(WS2812_PIN "$ENV{WS2812_PIN}")
if(NOT WS2812_PIN)
  set(WS2812_PIN 0)
endif()
add_definitions(-DWS2812_PIN=${WS2812_PIN})
message("Compiling with WS2812_PIN=${WS2812_PIN}")

if(PICO_AUTOTEST)
  add_definitions(-DPICO_AUTOTEST)
endif(PICO_AUTOTEST)


set(BEATLED_PROTOCOL_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/beatled_protocol")
include(beatled_protocol_import.cmake)
beatled_protocol_init()

# # Define ARM_CPU, CMSIS ROOT and DSP to use CMSIS-DSP
# set(ARM_CPU "cortex-m0plus")
# set(CMSISCORE ${CMAKE_CURRENT_LIST_DIR}/lib/CMSIS_5/CMSIS/Core)
# set(CMSISDSP ${CMAKE_CURRENT_LIST_DIR}/lib/CMSIS-DSP)

# add_subdirectory(${CMSISDSP}/Source bin_dsp EXCLUDE_FROM_ALL)
# add_subdirectory("lib/microphone-library-for-pico" EXCLUDE_FROM_ALL)

add_subdirectory(src)
add_subdirectory(tests)


