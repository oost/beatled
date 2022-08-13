cmake_minimum_required(VERSION 3.18)

project(RPIZBeatServer VERSION 0.1)

set (CMAKE_CXX_STANDARD 17)
set(BEAT_SERVER beat_server)

add_subdirectory(external)
add_subdirectory(lib)

add_subdirectory(src)
add_subdirectory(tests)


set(BINARY_INSTALL_DIR ${PROJECT_BINARY_DIR}/target)
set(INSTALL_TARGETS 
  ${BEAT_SERVER} 
  hello_world
  async_udp_echo_server
  blocking_udp_echo_server
  blocking_udp_echo_client
)

install(
  TARGETS ${INSTALL_TARGETS}
  DESTINATION ${BINARY_INSTALL_DIR}
)
