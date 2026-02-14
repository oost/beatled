
add_compile_definitions(SPDLOG_ACTIVE_LEVEL)
add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)

add_subdirectory(server)
add_subdirectory(core)
add_subdirectory(beat_detector)

# TODO: Break circular dependency between core and server.
# beatled_core depends on beatled_server for ServiceControllerInterface,
# and beatled_server depends on beatled_core for StateManager.
# Consider extracting shared interfaces into a separate target.
target_link_libraries(beatled_core PUBLIC
  beatled_server
  beatled_protocol
)


add_executable(${BEAT_SERVER} 
  beatled_server.cpp
  application.cpp
)


string(TIMESTAMP RPIZ_BS_BUILDTIME "%Y-%m-%d %H:%M")
configure_file(build_constants.h.in build_constants.h)
target_include_directories(${BEAT_SERVER} PUBLIC "${PROJECT_BINARY_DIR}/src")
target_link_libraries(${BEAT_SERVER} PRIVATE 
  beatled_server 
  beat_detector
  beatled_core

  Threads::Threads
  asio asio::asio 
  fmt::fmt
  bfg::lyra 
  spdlog::spdlog 
)


add_executable(beatled_cli beatled_cli.cpp)
target_link_libraries(beatled_cli PUBLIC 
  beat_detector
  portaudio_static
  # portaudio
  bfg::lyra 
  fmt::fmt
  spdlog::spdlog 

)


install(TARGETS beatled_cli DESTINATION bin)
install(TARGETS ${BEAT_SERVER} DESTINATION bin)