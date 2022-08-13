cmake_minimum_required(VERSION 3.10)

project(RPIZBeatServer VERSION 0.1)
set (CMAKE_CXX_STANDARD 17)
set(BEAT_SERVER beat_server)

add_executable(hello_world hello_world.cpp)

add_subdirectory(lib)
add_subdirectory(http)
add_subdirectory(beat)

add_executable(${BEAT_SERVER} main.cpp)


string(TIMESTAMP RPIZ_BS_BUILDTIME "%Y-%m-%d %H:%M")
configure_file(build_constants.h.in build_constants.h)
target_include_directories(${BEAT_SERVER} PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )
target_link_libraries(${BEAT_SERVER} PRIVATE http_handler beat_worker)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${BEAT_SERVER} PRIVATE Threads::Threads)


target_link_libraries(${BEAT_SERVER} PRIVATE fmt::fmt-header-only clara)


SET(BINARY_INSTALL_DIR ${PROJECT_BINARY_DIR}/target)

SET(INSTALL_TARGETS ${BEAT_SERVER} hello_world)

install(
  TARGETS ${INSTALL_TARGETS}
  DESTINATION ${BINARY_INSTALL_DIR}
)

# add_custom_command(
#   TARGET ${BEAT_SERVER}
#   POST_BUILD
#   COMMAND date +\"%s\" > ${TIMESTAMP}
#   VERBATIM
# )