add_library(beat_detector beat_detector.cpp)
target_link_libraries(beat_detector PRIVATE asio)

target_link_libraries(beat_detector PRIVATE portaudio_static)
target_link_libraries(beat_detector PRIVATE FFTW3::fftw3)
target_link_libraries(beat_detector PRIVATE AudioFile)
