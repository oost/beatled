
add_subdirectory(server)
add_subdirectory(state_manager)
add_subdirectory(config)
add_subdirectory(beat_detector)

add_executable(${BEAT_SERVER} beatled_server.cpp)


string(TIMESTAMP RPIZ_BS_BUILDTIME "%Y-%m-%d %H:%M")
configure_file(build_constants.h.in build_constants.h)
target_include_directories(${BEAT_SERVER} PUBLIC "${PROJECT_BINARY_DIR}/src")
target_link_libraries(${BEAT_SERVER} PRIVATE 
  beatled_server 
  beatled_state_manager 
  beat_detector
  beatled_config

  Threads::Threads
  asio asio::asio 
  fmt::fmt-header-only
  bfg::lyra 
  spdlog::spdlog 
  spdlog::spdlog_header_only
)


add_executable(beatled_cli beatled_cli.cpp)
target_link_libraries(beatled_cli PUBLIC 
  beat_detector
  portaudio_static 
  bfg::lyra 
  fmt::fmt-header-only 
)