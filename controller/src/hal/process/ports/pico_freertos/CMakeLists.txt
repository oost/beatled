target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
  process.c
)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  pico_stdlib
  FreeRTOS-Kernel
)
