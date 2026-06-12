# set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_TARGET_ARCHITECTURE aarch64-linux-gnu)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CXX_FLAGS -DNO_RECVMMSG)
set(VCPKG_C_FLAGS -DNO_RECVMMSG)

if(${PORT} MATCHES "portaudio")
    # set(VCPKG_LIBRARY_LINKAGE dynamic)
    set(PA_ALSA_DYNAMIC ON)
    # vcpkg restricts find_package() to its own tree, so PortAudio's
    # find_package(ALSA) misses the apt-installed system libasound and
    # silently ships a JACK-only build (0 host APIs on the Pi). This is a
    # native arm64 container build, so point FindALSA straight at the
    # system aarch64 libasound (libasound2-dev). With PA_ALSA_DYNAMIC the
    # lib is dlopen'd at runtime; we only need the headers + .so to build.
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS
        "-DPA_USE_ALSA=ON"
        "-DALSA_INCLUDE_DIR=/usr/include"
        "-DALSA_LIBRARY=/usr/lib/aarch64-linux-gnu/libasound.so")
endif()


if(NOT CMAKE_HOST_SYSTEM_PROCESSOR)
    execute_process(COMMAND "uname" "-m" OUTPUT_VARIABLE CMAKE_HOST_SYSTEM_PROCESSOR OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
