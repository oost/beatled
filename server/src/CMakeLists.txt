
add_compile_definitions(SPDLOG_ACTIVE_LEVEL)
add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)

add_subdirectory(server)
add_subdirectory(core)
add_subdirectory(beat_detector)

add_executable(${BEAT_SERVER}
  beatled_server.cpp
  application.cpp
)


add_custom_target(build_constants ALL
  COMMAND ${CMAKE_COMMAND}
    -DRPIZ_BS_VERSION_MAJOR=${RPIZBeatServer_VERSION_MAJOR}
    -DRPIZ_BS_VERSION_MINOR=${RPIZBeatServer_VERSION_MINOR}
    -DINPUT_FILE="${CMAKE_CURRENT_SOURCE_DIR}/build_constants.h.in"
    -DOUTPUT_FILE="${PROJECT_BINARY_DIR}/src/build_constants.h"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/generate_build_constants.cmake"
  COMMENT "Generating build_constants.h with current timestamp"
)
add_dependencies(${BEAT_SERVER} build_constants)
target_include_directories(${BEAT_SERVER} PUBLIC "${PROJECT_BINARY_DIR}/src")
# beatled_server PUBLIC-links beat_detector and beatled_core (which in turn
# expose fmt/spdlog/lyra/asio), so listing those here too just duplicates them
# on the link line. Keep only what isn't pulled transitively.
target_link_libraries(${BEAT_SERVER} PRIVATE
  beatled_server
  Threads::Threads
)


add_executable(beatled_cli beatled_cli.cpp)
# beat_detector PUBLIC-links portaudio_static + beatled_core (fmt/spdlog/lyra).
target_link_libraries(beatled_cli PRIVATE
  beat_detector
)


install(TARGETS beatled_cli DESTINATION bin)
install(TARGETS ${BEAT_SERVER} DESTINATION bin)