# FreeRTOS Migration Assessment for beatled-pico

## Context

The user is exploring what it would take to migrate the beatled-pico firmware from its current bare-metal dual-core architecture to FreeRTOS. This assessment analyzes the current architecture and identifies the work required for a FreeRTOS port.

## Current Architecture

### Concurrency Model
- **Core 0**: Runs networking, WiFi, and event processing
  - Blocking event loop (`run_event_loop`) that waits on a queue for UDP messages
  - lwIP runs in callback mode (NO_SYS=1) with `pico_cyw43_arch_lwip_threadsafe_background`
  - UDP receive callback (`dgram_recv`) enqueues messages to event queue
  - State machine processes events via `handle_event`

- **Core 1**: Dedicated to LED rendering
  - Tight loop: polls intercore queue → updates LEDs → sleep 10ms
  - Uses PIO+DMA for WS2812 output via `led_update()`
  - Receives tempo/program updates from Core 0 via intercore queue

### Key Dependencies
- **pico/multicore.h**: `multicore_launch_core1()` for core startup
- **pico/util/queue.h**: SDK's built-in inter-core queue (spinlock-based, multi-core safe)
- **pico_cyw43_arch_lwip_threadsafe_background**: WiFi + lwIP in background polling mode
- **lwIP NO_SYS mode**: Callback-based networking (no OS integration)

### HAL Abstractions
The code already has a Hardware Abstraction Layer (HAL) with ports for both `pico` and `posix`:
- `hal/queue`: Wraps SDK queue → would need FreeRTOS queue port
- `hal/process`: Manages core startup → would need task creation port
- `hal/network`: UDP/DNS via lwIP → would need lwIP+FreeRTOS port

### File Structure
```
src/
├── main.c                         # Entry point: state_manager_init() → core0_entry()
├── process/
│   ├── core0.c                    # Event loop on Core 0
│   ├── core1.c                    # LED loop on Core 1
│   └── intercore_queue.c          # Queue for Core 0 → Core 1 messages
├── hal/
│   ├── queue/ports/pico/queue.c   # Wraps pico/util/queue.h
│   ├── process/ports/pico/process.c  # multicore_launch_core1
│   └── network/ports/pico/*.c     # lwIP UDP server/client
└── event/
    └── event_loop.c               # Blocking queue wait + event dispatch
```

## Migration to FreeRTOS

### Option 1: FreeRTOS SMP (Symmetric Multi-Processing)

**Overview**: Use FreeRTOS SMP to run tasks across both cores with automatic load balancing.

**Changes Required**:

1. **Replace multicore with FreeRTOS tasks**
   - Convert `core0_entry()` → FreeRTOS task with normal priority
   - Convert `core1_loop()` → FreeRTOS task with high priority (pinned to Core 1)
   - Use `vTaskCoreAffinitySet()` to pin LED task to Core 1

2. **Replace SDK queues with FreeRTOS queues**
   - Port `hal/queue` to use `xQueueCreate()`, `xQueueSend()`, `xQueueReceive()`
   - Replace `event_queue` with FreeRTOS queue
   - Replace `intercore_command_queue` with FreeRTOS queue

3. **Reconfigure lwIP for FreeRTOS**
   - Change `NO_SYS=1` → `NO_SYS=0` in `lwipopts.h`
   - Enable lwIP threading: `LWIP_TCPIP_CORE_LOCKING=1`
   - Replace `pico_cyw43_arch_lwip_threadsafe_background` with `pico_cyw43_arch_lwip_sys_freertos`
   - Add `sys_arch.c` for FreeRTOS/lwIP glue (mailboxes, semaphores, threads)

4. **Modify CMakeLists.txt**
   - Add `FreeRTOS-Kernel` as submodule or library
   - Link `pico_cyw43_arch_lwip_sys_freertos` instead of `_threadsafe_background`
   - Add FreeRTOS config: `FreeRTOSConfig.h` with SMP enabled

5. **Timing changes**
   - Replace `sleep_ms(10)` in LED loop with `vTaskDelay(pdMS_TO_TICKS(10))`
   - Use FreeRTOS timers if needed for periodic tasks

6. **Startup sequence**
   ```c
   int main(void) {
     startup(&start_beatled);  // Hardware init

     xTaskCreate(core0_task, "Core0", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
     xTaskCreate(core1_task, "LED", 2048, NULL, tskIDLE_PRIORITY + 2, &led_task_handle);
     vTaskCoreAffinitySet(led_task_handle, (1 << 1));  // Pin to Core 1

     vTaskStartScheduler();  // Never returns
     return 0;
   }
   ```

**Pros**:
- Tasks can migrate between cores (if not pinned) → better load balancing
- Unified scheduling across both cores
- Rich FreeRTOS ecosystem (timers, event groups, semaphores, mutexes)
- Well-integrated lwIP FreeRTOS port

**Cons**:
- Overhead: FreeRTOS scheduler uses ~1-2KB RAM per task + kernel overhead
- LED rendering latency may increase due to preemption (mitigated by high priority)
- More complex than current bare-metal approach
- SMP configuration is more complex than single-core FreeRTOS

**Effort Estimate**: **Medium-High** (2-3 days)
- 4-6 hours: FreeRTOS integration, config, and build system
- 4-6 hours: Port HAL abstractions and networking
- 2-4 hours: Testing and timing validation

---

### Option 2: FreeRTOS on Core 0 Only (Asymmetric)

**Overview**: Run FreeRTOS scheduler only on Core 0, keep Core 1 bare-metal for LED rendering.

**Changes Required**:

1. **Core 0: Convert to FreeRTOS tasks**
   - Event loop → FreeRTOS task
   - State manager → FreeRTOS task (if split out)
   - lwIP runs in FreeRTOS mode on Core 0

2. **Core 1: Keep bare-metal**
   - Launch via `multicore_launch_core1()` as before
   - LED loop remains a tight `while(1)` with `sleep_ms()`
   - Inter-core queue can use SDK queue or FreeRTOS queue with proper locking

3. **lwIP configuration**
   - Same as Option 1, but only Core 0 participates in scheduling
   - CYW43 driver must coordinate with Core 1 (less clean)

4. **Synchronization**
   - FreeRTOS-aware spinlocks when Core 0 tasks talk to Core 1
   - Use `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` on Core 0 side
   - Use SDK spinlocks on Core 1 side

**Pros**:
- Core 1 LED rendering unaffected by scheduler → minimal latency
- Simpler than SMP FreeRTOS
- Smaller memory footprint (scheduler only on Core 0)

**Cons**:
- Asymmetric configuration is less common, more custom
- Inter-core synchronization becomes tricky (FreeRTOS-aware on one core, not the other)
- Benefits of FreeRTOS are limited to Core 0 only

**Effort Estimate**: **Medium** (1-2 days)

---

### Option 3: Bare-Metal with FreeRTOS Components (Hybrid)

**Overview**: Keep bare-metal dual-core but use FreeRTOS primitives (queues, timers) as libraries.

**Changes Required**:

1. **Use FreeRTOS queues as library**
   - Port `hal/queue` to FreeRTOS queue API
   - Don't start the scheduler (`vTaskStartScheduler()`)
   - Manually call `xPortGetTickCount()` and friends

2. **Keep multicore and event loops**
   - No change to core startup or loops
   - lwIP remains NO_SYS mode

**Pros**:
- Minimal disruption to current architecture
- Access to FreeRTOS queue/timer primitives
- No scheduler overhead

**Cons**:
- Very unconventional use of FreeRTOS (not recommended)
- Loses most benefits of an RTOS (scheduling, preemption, etc.)
- May be incompatible with some FreeRTOS APIs that expect scheduler

**Effort Estimate**: **Low** (0.5-1 day)
**Recommendation**: **Don't do this** — if you want FreeRTOS, use the scheduler.

---

## Recommendation

**If the goal is to use FreeRTOS**: Go with **Option 1 (FreeRTOS SMP)**.

**Rationale**:
- The Pico SDK has official FreeRTOS SMP support
- lwIP FreeRTOS integration is well-tested
- Provides a proper RTOS with scheduling, priorities, and resource management
- The LED task can be pinned to Core 1 with high priority to maintain timing

**Key Risks**:
- **LED timing**: The current bare-metal Core 1 loop has zero preemption. With FreeRTOS, the LED task can be preempted by higher-priority tasks or ISRs. Mitigation: Set LED task to highest priority and disable interrupts during critical PIO/DMA operations.
- **Memory**: FreeRTOS adds ~2KB per task + kernel overhead (~4-6KB). The Pico W has 264KB RAM, so this should be fine.
- **Complexity**: FreeRTOS configuration, especially SMP, requires careful tuning of stack sizes, priorities, and tick rate.

**If the goal is NOT to use FreeRTOS**: Stay with the current bare-metal architecture. It's already well-structured with HAL abstractions and works reliably for this use case.

---

## Why Migrate?

Before committing to this migration, clarify the motivation:

1. **Multi-tasking on Core 0?** If you want multiple concurrent tasks on Core 0 (e.g., separate networking, state machine, and sensor tasks), FreeRTOS provides clean task management.

2. **Better resource management?** FreeRTOS offers mutexes, semaphores, event groups, and timers that are cleaner than hand-rolled synchronization.

3. **Third-party library compatibility?** Some libraries expect an RTOS environment (e.g., certain network stacks, file systems).

4. **Porting to other platforms?** FreeRTOS is portable, so if you plan to move beyond the Pico, FreeRTOS provides a common abstraction.

**If none of these apply**, the current bare-metal architecture is simpler, faster, and easier to debug.

---

## Files to Modify (for Option 1 FreeRTOS SMP)

### Core Changes
- `CMakeLists.txt` — Add FreeRTOS-Kernel library, link `pico_cyw43_arch_lwip_sys_freertos`
- `src/main.c` — Convert to FreeRTOS tasks, call `vTaskStartScheduler()`
- `src/process/core0.c` — Convert `core0_loop()` to FreeRTOS task function
- `src/process/core1.c` — Convert `core1_loop()` to FreeRTOS task function, pin to Core 1
- `src/event/event_loop.c` — Replace SDK queue with FreeRTOS queue blocking API

### HAL Ports
- `src/hal/queue/ports/pico/queue.c` — Replace SDK queue with FreeRTOS queue API
- `src/hal/process/ports/pico/process.c` — Replace `multicore_launch_core1` with `xTaskCreate` + `vTaskCoreAffinitySet`
- `src/hal/network/ports/pico/CMakeLists.txt` — Link `pico_cyw43_arch_lwip_sys_freertos`
- `src/hal/lwipopts.h` — Set `NO_SYS=0`, `LWIP_TCPIP_CORE_LOCKING=1`

### New Files
- `FreeRTOSConfig.h` — FreeRTOS SMP configuration (tick rate, heap size, SMP enabled)
- `lib/FreeRTOS-Kernel/` — Add FreeRTOS as submodule or library

### Verification
- Build and flash to Pico W
- Connect to WiFi, verify UDP reception
- Check LED patterns render correctly in sync with beats
- Monitor task stats via `vTaskList()` or SEGGER SystemView
- Measure LED loop timing with GPIO toggles + logic analyzer to ensure <10ms jitter
