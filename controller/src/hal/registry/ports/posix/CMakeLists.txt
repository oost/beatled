target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
    registry.c
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  Threads::Threads
)
