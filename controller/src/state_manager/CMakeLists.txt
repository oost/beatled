add_library(beatled_state_manager INTERFACE)

target_sources(
    beatled_state_manager INTERFACE 
        state_manager.c
        states/started/started.c
        states/initialized/initialized.c
        states/registered/registered.c
        states/time_synced/time_synced.c
        states/tempo_synced/tempo_synced.c
)
target_compile_definitions(beatled_state_manager INTERFACE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    BEATLED_SERVER_NAME="${BEATLED_SERVER_NAME}"
)

target_link_libraries(beatled_state_manager INTERFACE
    # hardware_timer
    # hardware_clocks
    # pico_multicore
    beatled_hal_wifi
    beatled_hal_board
    
    beatled_commands
    beatled_ws2812
    beatled_config
)

target_include_directories(beatled_state_manager INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
