target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
    dns.c
    udp_sender.c
    udp_server.c
)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
  pico_stdlib
  pico_cyw43_arch_lwip_sys_freertos
)
# For lwipopts.h
target_include_directories(${CURRENT_LIBRARY_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../../..)
