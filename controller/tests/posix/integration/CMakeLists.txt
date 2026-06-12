add_executable(test_integration
    test_integration.cpp
    test_sync_convergence.cpp
    integration_stubs.c
    # Real state machine
    ${CMAKE_SOURCE_DIR}/src/state_manager/state_manager.c
    # Real state handlers (not stubs — they call into HAL stubs)
    ${CMAKE_SOURCE_DIR}/src/state_manager/states/started/started.c
    ${CMAKE_SOURCE_DIR}/src/state_manager/states/initialized/initialized.c
    ${CMAKE_SOURCE_DIR}/src/state_manager/states/registered/registered.c
    ${CMAKE_SOURCE_DIR}/src/state_manager/states/time_synced/time_synced.c
    ${CMAKE_SOURCE_DIR}/src/state_manager/states/tempo_synced/tempo_synced.c
    # Real command handlers
    ${CMAKE_SOURCE_DIR}/src/command/command.c
    ${CMAKE_SOURCE_DIR}/src/command/diagnostics/qos.c
    ${CMAKE_SOURCE_DIR}/src/command/hello/hello.c
    ${CMAKE_SOURCE_DIR}/src/command/time/time.c
    ${CMAKE_SOURCE_DIR}/src/command/tempo/tempo.c
    ${CMAKE_SOURCE_DIR}/src/command/next_beat/next_beat.c
    ${CMAKE_SOURCE_DIR}/src/command/status/status.c
    # Real clock and event queue
    ${CMAKE_SOURCE_DIR}/src/clock/clock.c
    ${CMAKE_SOURCE_DIR}/src/event/event_queue.c
)
# Compile definitions needed by started.c
target_compile_definitions(test_integration PRIVATE
    WIFI_SSID=\"test\"
    WIFI_PASSWORD=\"test\"
    WIFI_SSID_2=\"\"
    WIFI_PASSWORD_2=\"\"
    WIFI_SSID_3=\"\"
    WIFI_PASSWORD_3=\"\"
    WIFI_SSID_4=\"\"
    WIFI_PASSWORD_4=\"\"
    BEATLED_SERVER_NAME="localhost"
)
target_include_directories(test_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/src/state_manager/include
    ${CMAKE_SOURCE_DIR}/src/command/include
    ${CMAKE_SOURCE_DIR}/src/clock/include
    ${CMAKE_SOURCE_DIR}/src/event/include
    ${CMAKE_SOURCE_DIR}/src/process/include
    ${CMAKE_SOURCE_DIR}/src/config/include
    # HAL headers needed by state handlers and commands — the real
    # implementations are replaced by stubs in integration_stubs.c
    ${CMAKE_SOURCE_DIR}/src/hal/network/include
    ${CMAKE_SOURCE_DIR}/src/hal/blink/include
    ${CMAKE_SOURCE_DIR}/src/hal/wifi/include
    ${CMAKE_SOURCE_DIR}/src/hal/process/include
    ${CMAKE_SOURCE_DIR}/src/hal/time/include
    ${CMAKE_SOURCE_DIR}/src/hal/registry/include
)
target_link_libraries(test_integration PRIVATE
    Catch2::Catch2WithMain
    beatled_protocol
    beatled_hal_queue
    beatled_hal_board
    beatled_hal_registry
    beatled_hal_utils
    $<$<BOOL:${USE_FREERTOS}>:freertos_test_hooks>
)
if(NOT VCPKG_TARGET_TRIPLET)
    catch_discover_tests(test_integration)
endif()
