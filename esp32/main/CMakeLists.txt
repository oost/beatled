set(ROOT "${CMAKE_CURRENT_LIST_DIR}/../../src")
set(LIB "${CMAKE_CURRENT_LIST_DIR}/../../lib")

idf_component_register(
    SRCS
        # Entry point
        "app_main.c"
        "${ROOT}/main.c"

        # HAL — ESP32 port
        "${ROOT}/hal/runtime/ports/esp32/startup.c"
        "${ROOT}/hal/process/ports/esp32/process.c"
        "${ROOT}/hal/time/ports/esp32/time.c"
        "${ROOT}/hal/time/ports/posix_freertos/alarm.c"
        "${ROOT}/hal/queue/ports/pico_freertos/queue.c"
        "${ROOT}/hal/registry/ports/pico_freertos/registry.c"
        "${ROOT}/hal/network/ports/esp32/udp.c"
        "${ROOT}/hal/network/ports/posix/dns.c"
        "${ROOT}/hal/wifi/ports/esp32/wifi.c"
        "${ROOT}/hal/ws2812/ports/esp32/ws2812.c"
        "${ROOT}/hal/board/ports/esp32/unique_id.c"
        "${ROOT}/hal/blink/ports/esp32/blink.c"

        # Platform-independent: LED patterns
        "${ROOT}/ws2812/ws2812.c"
        "${ROOT}/ws2812/ws2812_patterns.c"
        "${ROOT}/ws2812/programs/greys/greys.c"
        "${ROOT}/ws2812/programs/drops/drops.c"
        "${ROOT}/ws2812/programs/fade/fade.c"
        "${ROOT}/ws2812/programs/solid/solid.c"
        "${ROOT}/ws2812/programs/random/random.c"
        "${ROOT}/ws2812/programs/sparkle/sparkle.c"
        "${ROOT}/ws2812/programs/snake/snake.c"

        # Platform-independent: state machine
        "${ROOT}/state_manager/state_manager.c"
        "${ROOT}/state_manager/states/started/started.c"
        "${ROOT}/state_manager/states/initialized/initialized.c"
        "${ROOT}/state_manager/states/registered/registered.c"
        "${ROOT}/state_manager/states/time_synced/time_synced.c"
        "${ROOT}/state_manager/states/tempo_synced/tempo_synced.c"

        # Platform-independent: commands
        "${ROOT}/command/command.c"
        "${ROOT}/command/time/time.c"
        "${ROOT}/command/tempo/tempo.c"
        "${ROOT}/command/hello/hello.c"
        "${ROOT}/command/next_beat/next_beat.c"

        # Platform-independent: clock, events, process, autotest
        "${ROOT}/clock/clock.c"
        "${ROOT}/event/event_loop.c"
        "${ROOT}/event/event_queue.c"
        "${ROOT}/process/intercore_queue.c"
        "${ROOT}/process/core0.c"
        "${ROOT}/process/isr_thread.c"
        "${ROOT}/process/core1.c"
        "${ROOT}/autotest/autotest.c"

        # Protocol library
        "${LIB}/beatled_protocol/include/beatled/protocol.c"

    INCLUDE_DIRS
        # HAL public headers
        "${ROOT}/hal/blink/include"
        "${ROOT}/hal/board/include"
        "${ROOT}/hal/network/include"
        "${ROOT}/hal/process/include"
        "${ROOT}/hal/queue/include"
        "${ROOT}/hal/registry/include"
        "${ROOT}/hal/runtime/include"
        "${ROOT}/hal/time/include"
        "${ROOT}/hal/wifi/include"
        "${ROOT}/hal/ws2812/include"
        "${ROOT}/hal/utils/include"

        # Application headers
        "${ROOT}/autotest"
        "${ROOT}/config/include"
        "${ROOT}/ws2812/include"
        "${ROOT}/state_manager/include"
        "${ROOT}/command/include"
        "${ROOT}/clock/include"
        "${ROOT}/event/include"
        "${ROOT}/process/include"

        # Internal port headers (dns.h)
        "${ROOT}/hal/network/ports/posix"

        # Protocol library
        "${LIB}/beatled_protocol/include"

    REQUIRES
        driver
        esp_driver_gpio
        esp_wifi
        esp_timer
        esp_netif
        led_strip
        lwip
        nvs_flash
)

# Build configuration from environment variables
set(WIFI_SSID "$ENV{WIFI_SSID}" CACHE STRING "WiFi SSID")
set(WIFI_PASSWORD "$ENV{WIFI_PASSWORD}" CACHE STRING "WiFi password")
set(BEATLED_SERVER_NAME "$ENV{BEATLED_SERVER_NAME}" CACHE STRING "Server hostname")
set(NUM_PIXELS "$ENV{NUM_PIXELS}" CACHE STRING "Number of LED pixels")
set(BEATLED_VERBOSE_LOG "$ENV{BEATLED_VERBOSE_LOG}" CACHE STRING "Enable verbose logging")
set(WS2812_PIN "$ENV{WS2812_PIN}" CACHE STRING "WS2812 LED data pin number")

if(NOT NUM_PIXELS)
    set(NUM_PIXELS 30)
endif()
if(NOT BEATLED_SERVER_NAME)
    set(BEATLED_SERVER_NAME "localhost")
endif()
if(NOT BEATLED_VERBOSE_LOG)
    set(BEATLED_VERBOSE_LOG 0)
endif()
if(NOT WS2812_PIN)
    set(WS2812_PIN 0)
endif()

target_compile_definitions(${COMPONENT_LIB} PUBLIC
    ESP32_PORT
    FREERTOS_PORT
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    BEATLED_SERVER_NAME="${BEATLED_SERVER_NAME}"
    NUM_PIXELS=${NUM_PIXELS}
    BEATLED_VERBOSE_LOG=${BEATLED_VERBOSE_LOG}
    WS2812_PIN=${WS2812_PIN}
)
