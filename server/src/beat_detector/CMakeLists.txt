add_library(
    beat_detector 
    beat_detector.cpp
    audio_input.cpp
    audio_output.cpp
)

target_link_libraries(
    beat_detector 
    PUBLIC 
      asio 
      portaudio_static 
      FFTW3::fftw3 
      AudioFile 
      btrack
)
