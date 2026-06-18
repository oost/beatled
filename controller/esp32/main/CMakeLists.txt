set(ROOT "${CMAKE_CURRENT_LIST_DIR}/../../src")
set(LIB "${CMAKE_CURRENT_LIST_DIR}/../../lib")

# Generate generated/beatled_version.h (git hash + build time), mirroring the
# Pico build (controller/CMakeLists.txt). main.c includes it. Done at configure
# time so the header exists before compilation.
set(BEATLED_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${BEATLED_GENERATED_DIR}")
execute_process(
  COMMAND ${CMAKE_COMMAND}
    -DBEATLED_SRC_DIR=${CMAKE_CURRENT_LIST_DIR}/../..
    -DOUT=${BEATLED_GENERATED_DIR}/beatled_version.h
    -P ${CMAKE_CURRENT_LIST_DIR}/../../cmake/gen_version.cmake
)

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
        "${ROOT}/ws2812/programs/off/off.c"

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
        "${ROOT}/command/status/status.c"
        "${ROOT}/command/diagnostics/qos.c"

        # Platform-independent: clock, events, process, autotest
        "${ROOT}/clock/clock.c"
        "${ROOT}/event/event_loop.c"
        "${ROOT}/event/event_queue.c"
        "${ROOT}/process/intercore_queue.c"
        "${ROOT}/process/core0.c"
        "${ROOT}/process/core1.c"
        "${ROOT}/autotest/autotest.c"

        # Protocol library
        "${LIB}/beatled_protocol/include/beatled/protocol.c"

    INCLUDE_DIRS
        # Generated version header (git hash + build time)
        "${BEATLED_GENERATED_DIR}"

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
# Optional fallback networks, tried in order if the first fails.
set(WIFI_SSID_2 "$ENV{WIFI_SSID_2}" CACHE STRING "WiFi SSID 2")
set(WIFI_PASSWORD_2 "$ENV{WIFI_PASSWORD_2}" CACHE STRING "WiFi password 2")
set(WIFI_SSID_3 "$ENV{WIFI_SSID_3}" CACHE STRING "WiFi SSID 3")
set(WIFI_PASSWORD_3 "$ENV{WIFI_PASSWORD_3}" CACHE STRING "WiFi password 3")
set(WIFI_SSID_4 "$ENV{WIFI_SSID_4}" CACHE STRING "WiFi SSID 4")
set(WIFI_PASSWORD_4 "$ENV{WIFI_PASSWORD_4}" CACHE STRING "WiFi password 4")
# Last-resort hotspot the controller joins when no upstream network connects.
set(HOTSPOT_SSID "$ENV{HOTSPOT_SSID}" CACHE STRING "Fallback hotspot SSID")
set(HOTSPOT_PASSWORD "$ENV{HOTSPOT_PASSWORD}" CACHE STRING "Fallback hotspot password")
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
    # ESP-IDF provides its own app_main(); don't compile the shared int main().
    BEATLED_PROVIDES_MAIN=0
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    WIFI_SSID_2=\"${WIFI_SSID_2}\"
    WIFI_PASSWORD_2=\"${WIFI_PASSWORD_2}\"
    WIFI_SSID_3=\"${WIFI_SSID_3}\"
    WIFI_PASSWORD_3=\"${WIFI_PASSWORD_3}\"
    WIFI_SSID_4=\"${WIFI_SSID_4}\"
    WIFI_PASSWORD_4=\"${WIFI_PASSWORD_4}\"
    HOTSPOT_SSID=\"${HOTSPOT_SSID}\"
    HOTSPOT_PASSWORD=\"${HOTSPOT_PASSWORD}\"
    BEATLED_SERVER_NAME="${BEATLED_SERVER_NAME}"
    NUM_PIXELS=${NUM_PIXELS}
    BEATLED_VERBOSE_LOG=${BEATLED_VERBOSE_LOG}
    WS2812_PIN=${WS2812_PIN}
)
