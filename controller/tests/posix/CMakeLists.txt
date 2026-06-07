if (POSIX_PORT)

  find_package(Catch2 3 REQUIRED)
  include(Catch)

  # FreeRTOS tests need vApplicationStackOverflowHook
  if(USE_FREERTOS)
    add_library(freertos_test_hooks STATIC freertos_test_hooks.c)
    target_link_libraries(freertos_test_hooks PUBLIC FreeRTOS-Kernel)
  endif()

  add_subdirectory(fibonacci)
  add_subdirectory(queue)
  add_subdirectory(clock)
  add_subdirectory(protocol)
  add_subdirectory(event_queue)
  add_subdirectory(state_manager)
  add_subdirectory(alarm)
  add_subdirectory(command)
  add_subdirectory(integration)
endif()