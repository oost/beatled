# target_link_libraries(beatled_hal_udp
#   pico_stdlib
#   pico_cyw43_arch_lwip_threadsafe_background
# )


add_hal_module(beatled_hal_network)


# target_include_directories(${CURRENT_LIBRARY_NAME} PRIVATE
#   ${CMAKE_CURRENT_LIST_DIR}/..
# )
