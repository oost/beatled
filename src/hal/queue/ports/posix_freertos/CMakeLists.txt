target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/../pico_freertos/queue.c
)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  FreeRTOS-Kernel
)
