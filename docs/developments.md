---
title: Further Developments
layout: default
nav_order: 7
---

# Further Developments

## Native iOS App

The current control panel is a React web app served over HTTPS. While this works well and can be installed as a PWA on iOS, a native Swift app would unlock several improvements:

- **Bluetooth provisioning** -- configure WiFi credentials on Pico W devices without needing a serial connection
- **On-device beat detection** -- use the iPhone microphone and Core ML to run beat tracking directly, removing the need for a Raspberry Pi server in simpler setups
- **Haptic feedback** -- tap along with the beat using the Taptic Engine
- **Widgets and Live Activities** -- show current tempo and beat count on the Lock Screen
- **Better offline support** -- native networking with Bonjour/mDNS for automatic server discovery on the local network
- **AirPlay / Spotify integration** -- sync to audio playback metadata instead of (or in addition to) microphone input

The server already exposes a REST API over HTTPS, so the first step would be a thin SwiftUI client that replaces the web dashboard. Beat detection on-device would be a larger effort, likely using the Accelerate framework for FFT and a Core ML port of the BTrack onset detection model.

---

## RTOS on the Pico W (FreeRTOS)

The Pico firmware currently manages concurrency manually: Core 0 runs an event loop (UDP listener, command handlers, state machine), Core 1 runs a tight LED update loop, and the two cores communicate through a lock-free intercore queue. This works, but has rough edges -- timer callbacks fire from ad-hoc pthreads (posix) or hardware alarm ISRs (pico), priority between networking and state transitions is implicit, and there is no preemption if a handler blocks.

[FreeRTOS has first-class support for the RP2040](https://www.freertos.org/symmetric-multiprocessing-introduction.html) via the Pico SDK's `pico_async_context_freertos` library, including SMP across both cores. Adopting it would give us:

- **Task priorities** -- the LED render task could run at a higher priority than networking, guaranteeing frame deadlines are met even during bursts of UDP traffic. The current architecture relies on Core 1 being fully dedicated to LEDs, which means we can never run anything else there.
- **Software timers** -- replace the current alarm thread/ISR approach (repeating pthreads on posix, hardware alarms on pico) with FreeRTOS timer service. This gives predictable scheduling with less platform-specific code.
- **Queues with blocking and timeout** -- the intercore queue and event queue could become FreeRTOS queues with built-in mutex protection, `xQueueSendToBack`/`xQueueReceive` with timeout, and ISR-safe variants. This replaces the hand-written circular buffer and its posix pthread shims.
- **Stack overflow detection** -- FreeRTOS can detect stack overflows at runtime, which would help catch issues in the current unchecked `while(1)` loops.

### What it would take to refactor

1. **Build system** -- add `pico_async_context_freertos` and `FreeRTOS-Kernel` to the CMake build. The Pico SDK already bundles FreeRTOS as a submodule (`lib/FreeRTOS-Kernel`); it just needs to be enabled via `PICO_CXX_ENABLE_EXCEPTIONS=0` and the right `FreeRTOSConfig.h`.

2. **Replace `core0_entry`/`core1_entry` with FreeRTOS tasks** -- instead of manually launching cores, create tasks with `xTaskCreate` and let the FreeRTOS SMP scheduler pin them or let them float across cores:
   - `task_event_loop` (priority: normal) -- processes the event queue
   - `task_led_render` (priority: high) -- runs `led_update()` at a fixed rate
   - `task_hello_timer`, `task_tempo_timer`, etc. -- replaced by FreeRTOS software timers

3. **Replace queues** -- swap `hal_queue_*` (circular buffer + pthread mutex) and `event_queue_*` with `xQueueCreate`/`xQueueSend`/`xQueueReceive`. The HAL abstraction layer already isolates queue usage behind `hal/queue.h`, so the API surface is small.

4. **Replace alarms** -- the posix port uses `pthread_create` per timer; the pico port uses hardware repeating timers. Both would become `xTimerCreate` with a callback, unifying the two ports.

5. **Posix port** -- FreeRTOS has a [POSIX/Linux simulator port](https://www.freertos.org/FreeRTOS-simulator-for-Linux.html) that runs tasks as pthreads with simulated scheduling. The posix HAL layer could target this, keeping the desktop simulator functional.

6. **Mutex for registry** -- replace the current `registry_lock_mutex`/`registry_unlock_mutex` (pico spinlock or pthread mutex) with a FreeRTOS mutex, gaining priority inheritance for free.

The HAL layer (`hal/queue.h`, `hal/time.h`, `hal/process.h`) was designed to isolate platform differences, so most of the refactoring would be contained within the `ports/` directories. Application code in `command/`, `state_manager/`, and `ws2812/` should remain largely untouched.

---

# Beat Tracking Frameworks

## "Traditional" Signals-based Beat Tracking

| Library                                                                 | Language | Bindings | Beat Tracking | Online Beat Tracking | Downbeat Tracking |
| ----------------------------------------------------------------------- | -------- | -------- | ------------- | -------------------- | ----------------- |
| [BTrack](https://github.com/adamstark/BTrack)                           | C++      | Python   | ✅            | ✅                   | ❌                |
| [Aubio](https://aubio.org)                                              | C        | Python   | ✅            | ❓                   | ❌                |
| [Essentia](https://essentia.upf.edu/tutorial_rhythm_beatdetection.html) | C++      | Python   | ✅            | ❓                   | ❌                |

## ML Based Beat Tracking

### Papers

- https://dida.do/blog/machine-learning-approaches-for-time-series
- https://paperswithcode.com/sota/online-beat-tracking-on-gtzan?p=a-novel-1d-state-space-for-efficient-music
  - [1d Space](https://paperswithcode.com/paper/a-novel-1d-state-space-for-efficient-music)
  - [Beatnet](https://paperswithcode.com/paper/beatnet-crnn-and-particle-filtering-for)
  - [Böck & Schedl](https://www.dafx.de/paper-archive/2011/Papers/31_e.pdf)
  - https://program.ismir2020.net/static/final_papers/223.pdf
  - https://archives.ismir.net/ismir2016/paper/000186.pdf
  - https://tempobeatdownbeat.github.io/tutorial/intro.html

Results: Offline results for Beatnet are rather disappointing compared to Btrack. In particular, because the model is trained on master copies with no noise, it struggles to identify beats on audio recorded with a microphone.

### Libraries

https://madmom.readthedocs.io/en/latest/
https://github.com/mjhydri/1D-StateSpace
