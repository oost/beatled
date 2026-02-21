# FreeRTOS Migration: Status vs. Proposed Plan
*2026-02-21*

Reference doc: `freertos-migration-2026-02-16.md` (Option 1: FreeRTOS SMP)

---

## Summary

The FreeRTOS SMP migration (Option 1) is **substantially complete**. All core architectural changes have been implemented. The implementation follows the same strategy as proposed but with a cleaner startup pattern and some additions not in the original plan.

---

## Item-by-Item Comparison

### Core Changes

| Proposed | Status | Notes |
|---|---|---|
| `CMakeLists.txt`: add FreeRTOS-Kernel, link `pico_cyw43_arch_lwip_sys_freertos` | ✅ Done | Implemented via `cmake/port.cmake` with `pico_freertos` port selector |
| `src/main.c`: create tasks + call `vTaskStartScheduler()` | ✅ Done (differently) | See startup sequence difference below |
| `src/process/core0.c`: convert to FreeRTOS task | ✅ Done | `core0_loop()` runs as a FreeRTOS task, launched via HAL |
| `src/process/core1.c`: convert to FreeRTOS task, pin to Core 1 | ✅ Done | `vTaskCoreAffinitySet(handle, 1 << 1)` in `hal/process/ports/pico_freertos/process.c` |
| `src/event/event_loop.c`: replace SDK queue with FreeRTOS blocking API | ✅ Done | `hal_queue_pop_message_blocking()` wraps `xQueueReceive(..., portMAX_DELAY)` |

### HAL Ports

| Proposed | Status | Notes |
|---|---|---|
| `hal/queue` pico_freertos port | ✅ Done | `hal/queue/ports/pico_freertos/queue.c` — full `xQueueCreate/Send/Receive` implementation |
| `hal/process` pico_freertos port | ✅ Done | `hal/process/ports/pico_freertos/process.c` — `xTaskCreate` + `vTaskCoreAffinitySet` |
| `hal/network`: link `pico_cyw43_arch_lwip_sys_freertos` | ✅ Done | Configured in port.cmake |
| `lwipopts.h`: `NO_SYS=0`, `LWIP_TCPIP_CORE_LOCKING=1` | ❓ Unverified | Not confirmed in code review — needs manual check |

### New Files

| Proposed | Status | Notes |
|---|---|---|
| `FreeRTOSConfig.h` | ✅ Done | SMP enabled: `configNUMBER_OF_CORES=2`, `configUSE_CORE_AFFINITY=1`, 1000 Hz tick, 65 KB heap |
| `lib/FreeRTOS-Kernel/` submodule | ✅ Done | Referenced in `cmake/port.cmake` as `../lib/FreeRTOS-Kernel` |

---

## Differences from Proposed Solution

### 1. Startup sequence (significant but intentional)

**Proposed:**
```c
int main(void) {
    startup(&start_beatled);
    xTaskCreate(core0_task, "Core0", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(core1_task, "LED",   2048, NULL, tskIDLE_PRIORITY + 2, &led_handle);
    vTaskCoreAffinitySet(led_handle, (1 << 1));
    vTaskStartScheduler();
}
```

**Implemented:**
```c
// main.c
startup(&start_beatled);  // delegates everything

// hal/runtime/ports/pico_freertos/startup.cpp
void startup(startup_main_t fn) {
    startup_main_fn = fn;
    xTaskCreate(main_task, "Main", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
    vTaskStartScheduler();
}
// The "Main" task calls start_beatled() → core0_entry()
// Core 1 task is created later via start_core1() in HAL
```

**Assessment:** The implementation is **cleaner**. Task creation is encapsulated inside the HAL runtime port, keeping `main.c` platform-agnostic. Core 1 task creation happens via `hal/process` rather than in `main()`. This is a better design than the proposal.

### 2. Stack sizes

| Task | Proposed | Implemented |
|---|---|---|
| Core 0 / Main | 2048 bytes | 4096 bytes |
| Core 1 / LED | 2048 bytes | 2048 bytes |
| UDP listener | (not in plan) | 4096 bytes |

Core 0 stack was doubled from the proposal — likely discovered during testing that 2048 was insufficient for the networking + event handling stack depth.

### 3. Task naming

| Proposed name | Actual name |
|---|---|
| "Core0" | "Main" |
| "LED" | "LED" (matches) |

Minor, no functional impact.

---

## Additions Beyond the Plan

These were not in the original proposal but have been implemented:

### posix_freertos port
A full `posix_freertos` port exists alongside `pico_freertos`, with its own:
- `hal/process/ports/posix_freertos/process.c` — `sleep_ms()` via `vTaskDelay()`
- `hal/network/ports/posix_freertos/udp.c` — UDP listener as a FreeRTOS task

This enables running the firmware on Linux/macOS for development/testing — a significant addition not discussed in the migration doc.

### Stack overflow hook
```c
// hal/runtime/ports/pico_freertos/startup.cpp
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("[FATAL] Stack overflow in task: %s\n", pcTaskName);
    while (1) {}
}
```
Good safety addition. `configCHECK_FOR_STACK_OVERFLOW=2` is set in `FreeRTOSConfig.h`.

### Registry mutex
```c
// hal/registry/ports/pico_freertos/registry.c
static SemaphoreHandle_t registry_mutex;  // xSemaphoreCreateMutex()
```
Protects the shared `registry_t` struct accessed from both cores. Not mentioned in the migration plan but necessary for correct operation under SMP.

---

## What Still Needs Verification

1. **`lwipopts.h`** — Confirm `NO_SYS=0` and `LWIP_TCPIP_CORE_LOCKING=1` are set for the pico_freertos port. This is a required change from the proposal and could cause subtle networking bugs if missed.

2. **Hardware verification checklist** (from original plan):
   - [ ] Build and flash to Pico W
   - [ ] Connect to WiFi, verify UDP reception
   - [ ] Check LED patterns render correctly in sync with beats
   - [ ] Monitor task stats via `vTaskList()`
   - [ ] Measure LED loop timing with GPIO toggles + logic analyzer (<10ms jitter target)

---

## Overall Assessment

The migration follows Option 1 (FreeRTOS SMP) as recommended. The key structural decisions match the proposal:
- SMP with both cores under FreeRTOS
- LED task pinned to Core 1 at high priority
- FreeRTOS queues throughout HAL
- `pico_cyw43_arch_lwip_sys_freertos` for networking

The implementation is in some ways *better* than the proposal — the HAL-encapsulated startup and the posix_freertos port are improvements not anticipated in the plan. The only open risk is verifying `lwipopts.h`.
