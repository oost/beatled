find_package(FFTW3 CONFIG REQUIRED)

add_executable(test_fftw test_fftw3.cpp)

target_link_libraries(test_fftw FFTW3::fftw3l)