cmake_minimum_required(VERSION 3.10)

project(RPIZBeatServer VERSION 0.1)

set (CMAKE_CXX_STANDARD 17)
set(BEAT_SERVER beat_server)

add_subdirectory(external)
add_subdirectory(lib)

add_subdirectory(src)


set(BINARY_INSTALL_DIR ${PROJECT_BINARY_DIR}/target)
set(INSTALL_TARGETS ${BEAT_SERVER} hello_world)

install(
  TARGETS ${INSTALL_TARGETS}
  DESTINATION ${BINARY_INSTALL_DIR}
)
