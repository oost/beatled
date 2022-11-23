message("Configuring tests")
find_package(Catch2 CONFIG REQUIRED)
include(Catch) 



add_subdirectory(fftw)
add_subdirectory(logger)

add_executable(async_udp_echo_server async_udp_echo_server.cpp)
add_executable(blocking_udp_echo_server blocking_udp_echo_server.cpp)
add_executable(blocking_udp_echo_client blocking_udp_echo_client.cpp)

target_link_libraries(async_udp_echo_server asio)
target_link_libraries(blocking_udp_echo_server asio)
target_link_libraries(blocking_udp_echo_client asio)


target_link_libraries(async_udp_echo_server Threads::Threads)
target_link_libraries(blocking_udp_echo_server Threads::Threads)
target_link_libraries(blocking_udp_echo_client Threads::Threads)

add_executable(test_factorials test_factorials.cpp)
target_link_libraries(test_factorials PRIVATE Catch2::Catch2 Catch2::Catch2WithMain)
# add_test (NAME TestFactorials COMMAND test_factorials)
# catch_discover_tests(test_factorials)


# add_compile_definitions(USE_FFTW)

add_executable(rpitest_btrack test_btrack.cpp)
target_link_libraries(rpitest_btrack PRIVATE btrack)

set(KISSFFT_STATIC ON)
find_package(unofficial-kissfft CONFIG REQUIRED)
set(FFT_LIBRARY unofficial::kissfft::kissfft)

add_executable(test_kissfft test_kissfft.cpp)
target_link_libraries(test_kissfft PRIVATE unofficial::kissfft::kissfft)


add_executable(test_audio_buffer_pool test_audio_buffer_pool.cpp)
target_link_libraries(test_audio_buffer_pool PRIVATE Catch2::Catch2 Catch2::Catch2WithMain spdlog::spdlog)
catch_discover_tests(test_audio_buffer_pool)

# add_executable(test_tempo test-tempo.c)
# target_link_libraries(test_tempo PRIVATE Aubio::aubio)

add_executable(test_move_semantics test_move_semantics.cpp)
target_link_libraries(test_move_semantics PRIVATE Catch2::Catch2 Catch2::Catch2WithMain)

catch_discover_tests(test_move_semantics)
