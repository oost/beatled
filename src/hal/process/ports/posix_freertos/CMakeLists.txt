target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
  process.c
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  FreeRTOS-Kernel
  Threads::Threads
)
