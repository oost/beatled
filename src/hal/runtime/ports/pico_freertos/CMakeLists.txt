target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/startup.cpp
)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  FreeRTOS-Kernel
  FreeRTOS-Kernel-Heap4
)
