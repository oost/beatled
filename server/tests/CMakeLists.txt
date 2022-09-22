add_subdirectory(fftw)

add_executable(async_udp_echo_server async_udp_echo_server.cpp)
add_executable(blocking_udp_echo_server blocking_udp_echo_server.cpp)
add_executable(blocking_udp_echo_client blocking_udp_echo_client.cpp)

target_link_libraries(async_udp_echo_server asio)
target_link_libraries(blocking_udp_echo_server asio)
target_link_libraries(blocking_udp_echo_client asio)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(async_udp_echo_server Threads::Threads)
target_link_libraries(blocking_udp_echo_server Threads::Threads)
target_link_libraries(blocking_udp_echo_client Threads::Threads)


find_package(Catch2 CONFIG REQUIRED)
add_executable(test_factorials test_factorials.cpp)
target_link_libraries(test_factorials PRIVATE Catch2::Catch2 Catch2::Catch2WithMain)


# add_compile_definitions(USE_FFTW)

add_executable(rpitest_btrack test_btrack.cpp)
target_link_libraries(rpitest_btrack PRIVATE btrack)


add_executable(test_kissfft test_kissfft.cpp)
target_link_libraries(test_kissfft PRIVATE kissfft)


# add_executable(test_tempo test-tempo.c)
# target_link_libraries(test_tempo PRIVATE Aubio::aubio)

