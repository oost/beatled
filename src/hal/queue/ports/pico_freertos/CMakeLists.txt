target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
  queue.c
)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  FreeRTOS-Kernel
)
