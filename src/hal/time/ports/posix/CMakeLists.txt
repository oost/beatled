target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
    time.c
    alarm.c
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  Threads::Threads
)
