# FreeRTOS: developments.md vs. Actual Implementation
*2026-02-21*

Compares the "RTOS on the Pico W (FreeRTOS)" section of `docs/developments.md` against the implemented `pico_freertos` / `posix_freertos` ports.

---

## Item-by-Item Status

### 1. Build system — Add FreeRTOS to CMake

> *"add `pico_async_context_freertos` and `FreeRTOS-Kernel` to the CMake build"*

| What was proposed | What was implemented | Status |
|---|---|---|
| `FreeRTOS-Kernel` submodule | `lib/FreeRTOS-Kernel`, wired in `cmake/port.cmake` | ✅ Done |
| `pico_async_context_freertos` | `pico_cyw43_arch_lwip_sys_freertos` | ⚠️ Different library |

**On the library difference:** `pico_async_context_freertos` is a general async context wrapper. `pico_cyw43_arch_lwip_sys_freertos` is the correct library for this project — it integrates the CYW43 WiFi driver *and* lwIP with FreeRTOS as a single unit. The implementation choice is **correct**; the developments.md named the wrong library.

---

### 2. Replace core entry points with FreeRTOS tasks

> *"`task_event_loop` (priority: normal) — processes the event queue"*
> *"`task_led_render` (priority: high) — runs `led_update()` at a fixed rate"*

| Proposed task | Implemented task | Priority | Core | Status |
|---|---|---|---|---|
| `task_event_loop` | "Main" (runs `core0_loop`) | `tskIDLE_PRIORITY + 1` | Any (Core 0) | ✅ Done |
| `task_led_render` | "LED" (runs `core1_loop`) | `tskIDLE_PRIORITY + 2` | Pinned to Core 1 | ✅ Done |

Task naming differs (`task_event_loop` → `"Main"`, `task_led_render` → `"LED"`) but the structure and pinning match the intent exactly.

---

### 3. Replace queues with FreeRTOS queues

> *"swap `hal_queue_*` with `xQueueCreate`/`xQueueSend`/`xQueueReceive`"*

✅ **Fully done.** `hal/queue/ports/pico_freertos/queue.c` wraps all four operations:
- `xQueueCreate` — `hal_queue_init()`
- `xQueueSend(..., 0)` — `hal_queue_add_message()` (non-blocking)
- `xQueueSend(..., portMAX_DELAY)` — `hal_queue_add_message_blocking()`
- `xQueueReceive(..., 0)` — `hal_queue_pop_message()` (non-blocking)
- `xQueueReceive(..., portMAX_DELAY)` — `hal_queue_pop_message_blocking()`

Both the intercore queue and event queue go through this HAL, so the migration is complete.

---

### 4. Replace alarms with FreeRTOS software timers

> *"the posix port uses `pthread_create` per timer; the pico port uses hardware repeating timers. Both would become `xTimerCreate` with a callback"*

❌ **Not done.** `FreeRTOSConfig.h` has software timers enabled (`configUSE_TIMERS=1`), but no `xTimerCreate` calls were found in the codebase. The alarm/timer implementation was not migrated to FreeRTOS timers.

This is the **only major item from developments.md that remains unimplemented.**

What likely still exists:
- Pico port: hardware repeating timers (`add_repeating_timer_ms` / hardware alarms)
- Posix port: `pthread_create`-based timer threads

The proposed benefit was unifying both ports under `xTimerCreate` — predictable scheduling with less platform-specific code.

---

### 5. Posix FreeRTOS port

> *"FreeRTOS has a [POSIX/Linux simulator port](https://www.freertos.org/FreeRTOS-simulator-for-Linux.html) that runs tasks as pthreads with simulated scheduling. The posix HAL layer could target this."*

⚠️ **Partially done, differently.** A `posix_freertos` port exists and uses FreeRTOS queues and tasks. However, it appears to use standard POSIX sockets and system primitives rather than the official FreeRTOS POSIX/Linux simulator scheduler (which emulates preemption via signals/pthreads).

The `posix_freertos` port gives you FreeRTOS queue/task *API* on desktop, but the fidelity of the scheduler emulation depends on whether the GCC_POSIX FreeRTOS port was used. The cmake config sets `FREERTOS_PORT="GCC_POSIX"` which *is* the official simulator — so this may be fully correct.

**Verdict:** Likely done correctly (GCC_POSIX port selected), but worth confirming the scheduler actually runs preemptively on the desktop build.

---

### 6. Mutex for registry

> *"replace `registry_lock_mutex`/`registry_unlock_mutex` (pico spinlock or pthread mutex) with a FreeRTOS mutex, gaining priority inheritance for free"*

✅ **Done.** `hal/registry/ports/pico_freertos/registry.c` uses `xSemaphoreCreateMutex()` with `xSemaphoreTake`/`xSemaphoreGive`. Priority inheritance is enabled via `configUSE_MUTEXES=1` in `FreeRTOSConfig.h`.

---

### 7. Stack overflow detection

> *"FreeRTOS can detect stack overflows at runtime"*

✅ **Done.** `configCHECK_FOR_STACK_OVERFLOW=2` in `FreeRTOSConfig.h`, with `vApplicationStackOverflowHook` implemented in `hal/runtime/ports/pico_freertos/startup.cpp`.

---

## Summary

| developments.md item | Status |
|---|---|
| FreeRTOS-Kernel in CMake | ✅ Done |
| pico_async_context_freertos | ⚠️ Used `pico_cyw43_arch_lwip_sys_freertos` instead (correct choice) |
| task_event_loop / task_led_render | ✅ Done (named "Main" / "LED") |
| LED task pinned to Core 1 at high priority | ✅ Done |
| Replace hal_queue with xQueue* | ✅ Done |
| Replace alarms with xTimerCreate | ❌ Not done |
| Posix FreeRTOS simulator port | ⚠️ Done via GCC_POSIX — confirm preemption works |
| Registry mutex → FreeRTOS mutex | ✅ Done |
| Stack overflow detection | ✅ Done |

---

## Outstanding Work

### High priority
- **Software timers** (`xTimerCreate`): migrate `hello_timer`, `tempo_timer`, and any other repeating timers away from hardware alarms (pico) and pthread threads (posix). This was the main architectural improvement proposed for the alarm subsystem.

### Worth verifying

- **Posix FreeRTOS preemption**: confirm `posix_freertos` desktop build runs the FreeRTOS GCC_POSIX scheduler with actual preemption, not just cooperative yielding.

### Verified ✅

- **`lwipopts.h`**: `NO_SYS=0` and `LWIP_TCPIP_CORE_LOCKING=1` are correctly set, gated behind `#ifdef FREERTOS_PORT` in `src/hal/lwipopts.h:25-39`. Also sets `SYS_LIGHTWEIGHT_PROT=1`. Single file handles both ports cleanly via compile-time flag.
- **Bonus**: `LWIP_DNS_SUPPORT_MDNS_QUERIES=1` is already set (line 12), meaning the Pico W can resolve `.local` hostnames via lwIP without any additional mDNS library — relevant to the planned mDNS discovery work.
