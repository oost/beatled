target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
    udp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../posix/udp_socket.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../posix/dns.c
)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  FreeRTOS-Kernel
  beatled_hal_utils
)
