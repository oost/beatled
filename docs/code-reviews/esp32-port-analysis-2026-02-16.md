# ESP32 Port Analysis: beatled-pico

## Context

beatled-pico is a beat-synchronized WS2812 LED controller running on Raspberry Pi Pico W. It communicates with a central beatled server over UDP/WiFi, receives beat timing data, and renders LED patterns in sync. The codebase uses a HAL (Hardware Abstraction Layer) with port-specific implementations under `src/hal/*/ports/pico/` and `src/hal/*/ports/posix/`.

This document analyzes what it would take to add an ESP32 port (`src/hal/*/ports/esp32/`).

---

## TL;DR

The existing HAL abstraction makes this very feasible. The project already supports two ports (pico, posix), so the architecture is designed for adding a third. The biggest work items are: replacing PIO+DMA LED driving with ESP32's RMT peripheral, swapping CYW43 WiFi for ESP-IDF WiFi, and adapting the bare-metal dual-core model to ESP-IDF's FreeRTOS-based tasking. lwIP is shared between both platforms, which is a major win for the networking layer.

---

## Component-by-Component Analysis

### 1. Build System (Medium effort)

| Pico | ESP32 |
|------|-------|
| CMake + pico-sdk | CMake + ESP-IDF (also CMake-based) |
| `pico_sdk_import.cmake` | ESP-IDF component system |
| ARM GCC toolchain | Xtensa GCC or RISC-V GCC |
| UF2 output | Binary/OTA flash |

**What changes:**
- New top-level CMake integration for ESP-IDF (`idf_component_register()` calls)
- Replace `pico_sdk_import.cmake` with ESP-IDF project includes
- WiFi credentials: ESP-IDF uses `menuconfig` / `sdkconfig` (or can still use compile defs)
- Output: `.bin` instead of `.uf2`

**What stays:** CMake as build system, C11/C++20, same source structure

### 2. WS2812 LED Driver (Medium effort) — `src/hal/ws2812/`

| Pico | ESP32 |
|------|-------|
| PIO state machine (4 instructions) | RMT (Remote Control) peripheral |
| DMA feeding PIO FIFO | RMT has built-in DMA (ESP32-S3) or software encoder |
| Custom `.pio` program | RMT encoder API (`rmt_new_ws2812_encoder()`) |
| Semaphore + alarm for reset delay | RMT handles reset timing automatically |

**What changes:**
- New file: `src/hal/ws2812/ports/esp32/ws2812.c`
- Replace PIO+DMA with RMT channel configuration
- ESP-IDF 5.x has a dedicated WS2812 RMT encoder example
- RMT handles the 800kHz timing, T0H/T0L/T1H/T1L, and reset delay natively
- Double-buffering logic can stay the same (buffer swap, write to RMT)
- The `ws2812_dma.c` semaphore/alarm pattern maps to RMT's `rmt_transmit()` with done callback

**What stays:** Color buffer format (uint32_t GRB), pixel count config, all pattern code (`src/ws2812/programs/*`), gamma table

**ESP-IDF RMT approach:**
```c
rmt_channel_handle_t led_chan;
rmt_tx_channel_config_t tx_config = {
    .gpio_num = WS2812_PIN,
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = 10000000,  // 10MHz
    .mem_block_symbols = 64,
    .trans_queue_depth = 4,
};
rmt_new_tx_channel(&tx_config, &led_chan);
// + ws2812 encoder setup
```

### 3. WiFi (Medium effort) — `src/hal/wifi/`

| Pico | ESP32 |
|------|-------|
| CYW43 chip (external, SPI-connected) | Built-in WiFi (Xtensa + WiFi coprocessor) |
| `cyw43_arch_init()` | `esp_wifi_init()` + event loop |
| `cyw43_arch_wifi_connect_timeout_ms()` | Event-driven: `WIFI_EVENT_STA_CONNECTED` |
| Blocking connect | Event-based connect (can be wrapped as blocking) |

**What changes:**
- New file: `src/hal/wifi/ports/esp32/wifi.c`
- ESP-IDF WiFi uses an event loop model — need to adapt the `wifi_connect()` / `wifi_is_connected()` HAL interface
- NVS (Non-Volatile Storage) initialization required before WiFi
- WiFi credentials via `sdkconfig` or compile-time defines

**HAL interface to implement:**
```c
int wifi_connect(const char *ssid, const char *password);
bool wifi_is_connected(void);
```

Both are straightforward to implement with ESP-IDF's WiFi station mode.

### 4. Networking / UDP (Low effort) — `src/hal/network/`

| Pico | ESP32 |
|------|-------|
| lwIP (raw API, NO_SYS=1) | lwIP (socket API or raw API, with FreeRTOS) |
| `udp_new()`, `udp_recv()`, `udp_sendto()` | Same lwIP API available, or BSD sockets |

**What changes:**
- Can use the exact same lwIP raw API calls (ESP-IDF bundles lwIP)
- OR switch to BSD socket API (ESP-IDF enables `LWIP_SOCKET` by default)
- DNS resolution: `dns_gethostbyname()` works the same in both
- May need minor `lwipopts.h` adjustments for ESP-IDF defaults

**This is the easiest component** — lwIP is shared between both platforms. The UDP server/client code and DNS resolution may need minimal or no changes.

### 5. Multi-Core / Process (Medium effort) — `src/hal/process/`

| Pico | ESP32 |
|------|-------|
| Bare-metal dual core | FreeRTOS (mandatory in ESP-IDF) |
| `multicore_launch_core1(fn)` | `xTaskCreatePinnedToCore(fn, ..., core_id)` |
| No RTOS, direct core control | FreeRTOS tasks with priorities |
| `sleep_ms()` (busy wait or WFI) | `vTaskDelay(pdMS_TO_TICKS(ms))` |

**What changes:**
- New file: `src/hal/process/ports/esp32/process.c`
- Core 0 task: networking + event loop (pinned to core 0)
- Core 1 task: LED rendering (pinned to core 1)
- Replace `sleep_ms()` with `vTaskDelay()`
- Task stack sizes need tuning (ESP32 stacks are heap-allocated)

**Architecture maps well:** Both chips are dual-core. The existing Core 0 / Core 1 split translates directly.

### 6. Inter-Core Queue (Low effort) — `src/hal/queue/`

| Pico | ESP32 |
|------|-------|
| Pico SDK `queue_t` | FreeRTOS `xQueueCreate()` / `xQueueSend()` / `xQueueReceive()` |
| Spin-lock based | ISR-safe, priority-aware |

**What changes:**
- New file: `src/hal/queue/ports/esp32/queue.c`
- Direct mapping: `queue_add_blocking()` → `xQueueSend()`, `queue_try_remove()` → `xQueueReceive(0)`
- FreeRTOS queues are thread-safe and ISR-safe by default

### 7. Registry / Mutex (Low effort) — `src/hal/registry/`

| Pico | ESP32 |
|------|-------|
| Pico SDK `mutex_t` | FreeRTOS `SemaphoreCreateMutex()` |
| `mutex_enter_blocking()` | `xSemaphoreTake(mutex, portMAX_DELAY)` |

**What changes:**
- Straightforward 1:1 mapping of mutex primitives

### 8. Time & Alarms (Low effort) — `src/hal/time/`

| Pico | ESP32 |
|------|-------|
| `time_us_64()` | `esp_timer_get_time()` (also returns int64_t microseconds) |
| `add_repeating_timer_us()` | `esp_timer_create()` + `esp_timer_start_periodic()` |
| Hardware alarm for WS2812 reset | ESP timer or RMT handles this |

**What changes:**
- New file: `src/hal/time/ports/esp32/time.c`
- Near-direct mapping — both return 64-bit microsecond timestamps
- Repeating timers: ESP-IDF's `esp_timer` API is very similar

### 9. Board / Blink (Trivial) — `src/hal/board/`, `src/hal/blink/`

| Pico | ESP32 |
|------|-------|
| `pico_get_unique_board_id()` | `esp_efuse_mac_get_default()` (MAC as unique ID) |
| CYW43 LED (WiFi chip GPIO) | Standard GPIO for onboard LED (varies by board) |

**What changes:**
- Board ID: Use MAC address or eFuse ID
- Blink: Use `gpio_set_level()` on the board's LED pin (GPIO2 on many DevKit boards)

### 10. Runtime / Startup (Low effort) — `src/hal/runtime/`

**What changes:**
- ESP-IDF entry point is `app_main()` instead of `main()`
- NVS flash init, WiFi event loop init happen before application logic
- New file: `src/hal/runtime/ports/esp32/startup.c`

---

## What Stays Completely Unchanged

These modules have **zero hardware dependencies** and work as-is:

| Module | Path | Reason |
|--------|------|--------|
| LED patterns | `src/ws2812/programs/*` | Pure math on color buffers |
| Pattern dispatcher | `src/ws2812/ws2812_patterns.c` | Buffer manipulation only |
| Protocol handlers | `src/command/*` | Operates on message structs |
| State machine | `src/state_manager/*` | Pure logic |
| Event system | `src/event/*` | Uses HAL queue abstraction |
| Clock sync | `src/clock/*` | Pure arithmetic |
| Intercore queue logic | `src/process/intercore_queue.c` | Uses HAL queue |
| Protocol library | `lib/beatled_protocol/` | Header-only structs |
| Config constants | `src/config/*` | Compile-time values |

**This is roughly 60-70% of the codebase.**

---

## ESP32 Variant Considerations

| Variant | Cores | WiFi | BLE | RMT Channels | RAM | Price |
|---------|-------|------|-----|---------------|-----|-------|
| **ESP32** (original) | 2x Xtensa LX6 | Yes | Yes | 8 | 520KB | ~$3 |
| **ESP32-S2** | 1x Xtensa LX7 | Yes | No | 4 | 320KB | ~$2 |
| **ESP32-S3** | 2x Xtensa LX7 | Yes | Yes | 4 (with DMA) | 512KB | ~$3 |
| **ESP32-C3** | 1x RISC-V | Yes | Yes | 2 | 400KB | ~$1.5 |
| **ESP32-C6** | 1x RISC-V | Yes (WiFi 6) | Yes | 2 | 512KB | ~$2 |

**Recommendation: ESP32-S3** — dual-core (matches Pico architecture), RMT with DMA (best for WS2812), plenty of RAM, WiFi + BLE, good ESP-IDF support, similar price point to Pico W.

**ESP32-C3/C6 also viable** if single-core is acceptable (LED rendering and networking on one core with FreeRTOS preemptive scheduling). Cheaper option.

---

## Effort Estimate

| Component | New Files | Effort | Notes |
|-----------|-----------|--------|-------|
| Build system (CMake/ESP-IDF) | 2-3 | Medium | CMakeLists, sdkconfig, partition table |
| WS2812 (RMT driver) | 1-2 | Medium | RMT encoder + transmit, replace PIO+DMA |
| WiFi | 1 | Medium | Event-loop model differs from CYW43 |
| UDP/Network | 1 | Low | lwIP shared, minimal changes |
| Process (dual-core) | 1 | Medium | FreeRTOS tasks pinned to cores |
| Queue | 1 | Low | FreeRTOS queue, direct mapping |
| Registry/Mutex | 1 | Low | FreeRTOS semaphore |
| Time/Alarms | 1 | Low | `esp_timer` API is nearly identical |
| Board/Blink | 2 | Trivial | MAC address ID, GPIO LED |
| Runtime/Startup | 1 | Low | `app_main()` + NVS/WiFi init |
| **Total** | **~12-15 new files** | | |

**Estimated scope:** ~1000-1500 lines of new ESP32-specific code across HAL port files, plus build system changes. No changes to the ~60-70% of platform-independent code.

---

## Risks & Gotchas

1. **lwIP configuration differences**: ESP-IDF runs lwIP with FreeRTOS (`NO_SYS=0`), while Pico uses `NO_SYS=1`. The raw lwIP API behaves slightly differently in threaded mode — may need to use `LOCK_TCPIP_CORE()` or switch to socket API.

2. **WiFi event model**: CYW43 uses a simple blocking connect. ESP-IDF WiFi is fully event-driven. The HAL wrapper needs to bridge this (e.g., use a semaphore to block until connected).

3. **Flash partitioning**: ESP32 needs a partition table for OTA, NVS, and app firmware. Pico just has a flat UF2.

4. **Stack sizes**: FreeRTOS tasks need explicit stack allocation. The LED rendering task's stack needs to be large enough for pattern computation.

5. **GPIO numbering**: ESP32 GPIO numbers differ. The `WS2812_PIN` constant needs to be configurable per board variant.

6. **Watchdog timer**: ESP-IDF enables a task watchdog by default. Long-running loops without yielding will trigger a reset. Core 1's LED loop must call `vTaskDelay()` (already does `sleep_ms(10)`).

7. **WiFi + BLE coexistence**: If BLE is desired later, ESP32's coexistence scheduler can impact WiFi latency. Not an issue for UDP but worth noting.

---

## Verification Plan

1. Build the ESP32 port with ESP-IDF toolchain (`idf.py build`)
2. Flash to an ESP32-S3 DevKit (`idf.py flash`)
3. Verify WiFi connection and UDP communication with the beatled server
4. Verify WS2812 LED strip lights up with correct colors
5. Verify beat synchronization timing accuracy (compare with existing Pico device)
6. Run existing POSIX unit tests (unchanged code should still pass)
7. Measure LED update jitter on ESP32 vs Pico (should be <1ms)
