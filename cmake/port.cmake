
if(PORT STREQUAL "pico")
  set(PICO_PORT ON)
  set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/../lib/pico-sdk")
  include(pico_sdk_import.cmake)
elseif(PORT STREQUAL "pico_freertos")
  set(PICO_PORT ON)
  set(FREERTOS_PORT ON)
  set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/../lib/pico-sdk")
  set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_LIST_DIR}/../lib/FreeRTOS-Kernel")
  set(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.." CACHE STRING "")
  include(pico_sdk_import.cmake)
  include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)
elseif(PORT STREQUAL "posix_freertos")
  set(POSIX_PORT ON)
  set(USE_FREERTOS ON)
  set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_LIST_DIR}/../lib/FreeRTOS-Kernel")
  set(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.." CACHE STRING "")
  set(FREERTOS_PORT "GCC_POSIX" CACHE STRING "")
  set(FREERTOS_HEAP "4" CACHE STRING "")
elseif(PORT STREQUAL "posix")
  SET(POSIX_PORT ON)
else()
  message(FATAL_ERROR "Need to specify port. Got PORT='${PORT}'")
endif()

message("Building ${PORT} port")


macro(port_init)
  if (PICO_PORT)
    message("Initializing PICO SDK")
    pico_sdk_init()

    add_compile_definitions(PICO_PORT)

    if(FREERTOS_PORT)
      message("Initializing FreeRTOS")
      add_compile_definitions(FREERTOS_PORT)
    endif()
  endif()

  if(POSIX_PORT)
    message("Initializing POSIX configurations")

    add_compile_definitions(POSIX_PORT)

    if(APPLE)
      add_subdirectory(lib/metal)
    endif()

    if(STDC_NO_THREADS)
      message( FATAL_ERROR "Need thread support" )
    else()
      message("Threads are supported.")
    endif()

    if(USE_FREERTOS)
      message("Initializing FreeRTOS (POSIX)")
      add_compile_definitions(FREERTOS_PORT)
      add_subdirectory(${FREERTOS_KERNEL_PATH} ${CMAKE_BINARY_DIR}/freertos_kernel)
      # Alias targets to match RP2040 naming used by HAL CMakeLists.txt files
      add_library(FreeRTOS-Kernel ALIAS freertos_kernel)
      add_library(FreeRTOS-Kernel-Heap4 ALIAS freertos_kernel)
    endif()

  endif()
endmacro()
