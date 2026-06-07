target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
    udp.c
    udp_socket.c
    dns.c
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  Threads::Threads
  beatled_hal_utils
)


