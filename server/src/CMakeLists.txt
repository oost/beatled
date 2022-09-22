add_executable(hello_world hello_world.cpp)

add_subdirectory(server)
add_subdirectory(state_manager)
add_subdirectory(beat_detector)

add_executable(${BEAT_SERVER} main.cpp)


string(TIMESTAMP RPIZ_BS_BUILDTIME "%Y-%m-%d %H:%M")
configure_file(build_constants.h.in build_constants.h)
target_include_directories(${BEAT_SERVER} PUBLIC "${PROJECT_BINARY_DIR}/src")
target_link_libraries(${BEAT_SERVER} PRIVATE server state_manager beat_detector)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


target_link_libraries(${BEAT_SERVER} PRIVATE Threads::Threads)

target_link_libraries(${BEAT_SERVER} PRIVATE asio asio::asio fmt::fmt-header-only bfg::lyra spdlog::spdlog spdlog::spdlog_header_only)


add_executable(hello_world_fftw hello_world_fftw.cpp)
target_link_libraries(hello_world_fftw FFTW3::fftw3)



add_executable(hw_portaudio hello_world_portaudio.cpp)
target_link_libraries(hw_portaudio portaudio_static FFTW3::fftw3)


add_executable(audio_beat audio_beat.cpp)
target_link_libraries(audio_beat PUBLIC beat_detector portaudio_static bfg::lyra fmt::fmt-header-only )