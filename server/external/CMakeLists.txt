set(KISSFFT_PKGCONFIG OFF)
set(KISSFFT_STATIC ON)
set(KISSFFT_TEST OFF)
set(KISSFFT_TOOLS OFF)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

add_subdirectory(kissfft)

set(ENABLE_FFTW ON)
add_subdirectory(BTrack)

# add_library(asio INTERFACE)
# target_include_directories(asio INTERFACE ${CMAKE_CURRENT_LIST_DIR}/asio/asio/include)

# add_subdirectory(Lyra)
# add_subdirectory(fmt)
# add_subdirectory(json)
# add_subdirectory(spdlog)





