add_library(
    beat_detector 
    beat_detector.cpp
    audio/audio_input.cpp
    audio/audio_output.cpp
    audio/audio_recorder.cpp
    audio/audio_player.cpp
    audio/audio_interface.cpp
)

target_link_libraries(beat_detector 
    PUBLIC 
      asio 
      portaudio_static 
    #   portaudio
      FFTW3::fftw3 
      AudioFile 
      btrack
      beatled_core
      spdlog::spdlog
)

target_include_directories(beat_detector PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)