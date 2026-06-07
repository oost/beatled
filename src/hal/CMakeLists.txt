if(FREERTOS_PORT)
  set(PICO_WIFI_LIBRARY pico_cyw43_arch_lwip_sys_freertos)
else()
  set(PICO_WIFI_LIBRARY pico_cyw43_arch_lwip_threadsafe_background)
endif()

add_subdirectory(utils)

add_subdirectory(blink)
add_subdirectory(wifi)
add_subdirectory(queue)
add_subdirectory(ws2812)
add_subdirectory(network)
add_subdirectory(time)
add_subdirectory(process)
add_subdirectory(board)
add_subdirectory(runtime)
add_subdirectory(registry)

add_library(beatled_hal INTERFACE)

target_link_libraries(beatled_hal INTERFACE 
  beatled_hal_blink
  beatled_hal_time
  beatled_hal_wifi
  beatled_hal_network
  beatled_hal_queue
  beatled_hal_process
  beatled_hal_board
  beatled_hal_runtime
  beatled_hal_registry
)