add_executable(hello_world hello_world.cpp)

add_subdirectory(http)
add_subdirectory(beat)
add_subdirectory(state_manager)

add_executable(${BEAT_SERVER} main.cpp)


string(TIMESTAMP RPIZ_BS_BUILDTIME "%Y-%m-%d %H:%M")
configure_file(build_constants.h.in build_constants.h)
target_include_directories(${BEAT_SERVER} PUBLIC "${PROJECT_BINARY_DIR}/src")
target_link_libraries(${BEAT_SERVER} PRIVATE http_handler beat_worker state_manager)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${BEAT_SERVER} PRIVATE Threads::Threads)

target_link_libraries(${BEAT_SERVER} PRIVATE fmt::fmt-header-only lyra)
