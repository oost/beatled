add_library(
    beat_detector 
    beat_detector.cpp
    audio_input.cpp
    audio_output.cpp
    audio_recorder.cpp
    audio_player.cpp
)

target_link_libraries(beat_detector 
    PUBLIC 
      asio 
      portaudio_static 
    #   portaudio
      FFTW3::fftw3 
      AudioFile 
      btrack
      beatled_state_manager
      spdlog::spdlog
)

target_include_directories(beat_detector PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)