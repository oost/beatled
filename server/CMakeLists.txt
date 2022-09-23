cmake_minimum_required(VERSION 3.18)

project(RPIZBeatServer VERSION 0.1)

set (CMAKE_CXX_STANDARD 20)
set(BEAT_SERVER beat_server)
set(BUILD_SHARED_LIBS OFF)

find_package(FFTW3 CONFIG REQUIRED)
find_package(FFTW3l CONFIG REQUIRED)
find_package(unofficial-http-parser CONFIG REQUIRED)
find_package(lyra CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(restinio CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(date CONFIG REQUIRED)
find_package(portaudio CONFIG REQUIRED)
find_package(AudioFile CONFIG REQUIRED)


add_subdirectory(external)
# add_subdirectory(lib)

add_subdirectory(src)
add_subdirectory(tests)

enable_testing ()
add_test (NAME TestFactorials COMMAND test_factorials)

set(BINARY_INSTALL_DIR ${PROJECT_BINARY_DIR}/target)
set(INSTALL_TARGETS 
  ${BEAT_SERVER} 
  hello_world
  hello_world_fftw
  async_udp_echo_server
  blocking_udp_echo_server
  blocking_udp_echo_client
)

install(
  TARGETS ${INSTALL_TARGETS}
  DESTINATION ${BINARY_INSTALL_DIR}
)
