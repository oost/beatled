# set(VCPKG_TARGET_TRIPLET arm64-linux)

if (VCPKG_DIR)
  if (VCPKG_TARGET_TRIPLET)
    message("VCPKG_DIR=${VCPKG_DIR}")
    include ("${VCPKG_DIR}/scripts/cmake/vcpkg_configure_cmake.cmake")

    set(CMAKE_SYSTEM_NAME "Linux")
    if(NOT VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
      message("Selecting toolchain based on triplet")
      set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${VCPKG_DIR}/scripts/toolchains/linux.cmake")
    else()
      message("Using toolchain from command line")
    endif()
    message("Using toolchain: ${VCPKG_CHAINLOAD_TOOLCHAIN_FILE}")

    message("Loading triplet file for ${VCPKG_TARGET_TRIPLET}")
    if(EXISTS "${VCPKG_DIR}/triplets/${VCPKG_TARGET_TRIPLET}.cmake")
      include("${VCPKG_DIR}/triplets/${VCPKG_TARGET_TRIPLET}.cmake")
    elseif(EXISTS "${VCPKG_DIR}/triplets/community/${VCPKG_TARGET_TRIPLET}.cmake")
      include("${VCPKG_DIR}/triplets/community/${VCPKG_TARGET_TRIPLET}.cmake")
    else()
      message(FATAL_ERROR "Couldn't find triplet file for ${VCPKG_TARGET_TRIPLET}")
    endif()

  else()
    message("No triplet was passed. Compiling for host.")
  endif()

  message("Loading VCPKG cmake file")
  include("${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake")
endif()


