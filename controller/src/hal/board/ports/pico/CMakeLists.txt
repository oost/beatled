target_sources(${CURRENT_LIBRARY_NAME} INTERFACE
    unique_id.c
    board.c
)

target_link_libraries(${CURRENT_LIBRARY_NAME} INTERFACE
    pico_stdlib
    pico_unique_id
#   pico_cyw43_arch_lwip_threadsafe_background
)
