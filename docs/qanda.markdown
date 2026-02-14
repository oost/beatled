---
title: Q&A
layout: default
nav_order: 9
---

# Q&A

### Why build the server in C++ and not in Python?

The server needs to capture audio, run beat detection, serve an HTTPS API, and broadcast UDP packets -- all in real time on a Raspberry Pi. C++ lets us keep end-to-end latency low (audio callback to UDP broadcast in under a millisecond) and memory usage small enough to run comfortably on a Pi Zero or Pi 4. Python would add GIL contention between the audio thread and networking, and the overhead of NumPy/SciPy FFTs is measurably higher than FFTW on ARM. Since the server runs headless 24/7, a compiled binary with no runtime dependencies is also easier to deploy.

### Why use BTrack for beat detection?

We evaluated several libraries (see [Further Developments](developments.html) for the full comparison). BTrack stood out because:

- It is designed for **online (real-time) beat tracking**, not offline analysis. It processes audio frame-by-frame and produces beat events with minimal lookahead.
- It runs efficiently on ARM -- the core algorithm is an onset detection function (spectral difference) fed into an adaptive tempo estimator and beat predictor, all using a single FFT per hop.
- It is written in C++ with no heavy dependencies (no TensorFlow, no PyTorch).

We also tested ML-based trackers like BeatNet. While they score higher on offline benchmarks, they struggle with live microphone audio that includes room noise and reverb -- exactly the conditions Beatled operates in.

### Why UDP instead of TCP for device communication?

Beat timing is extremely latency-sensitive. A single retransmission delay from TCP could cause an LED flash to arrive a full beat late. UDP gives us fire-and-forget delivery with no head-of-line blocking. If a packet is lost, the next NEXT_BEAT message arrives within one beat period anyway, so the device self-corrects. The protocol is also simple enough (packed structs, no fragmentation) that reliability at the transport layer is unnecessary.

### How accurate is the time synchronization?

The Pico devices use an NTP-style symmetric exchange (see [Protocol](protocol.html)) to calculate clock offset with the server. Each round measures the offset as `((T2-T1) + (T3-T4)) / 2`, canceling out symmetric network delay. On a local WiFi network, a single round typically achieves accuracy within 1-2 ms. Multiple rounds are performed during the TIME_SYNCED state to improve precision, and periodic re-syncs during TEMPO_SYNCED prevent clock drift.

### Why a Raspberry Pi Pico W and not an ESP32?

The Pico W has a dual-core ARM Cortex-M0+ processor, which maps cleanly to the architecture: Core 0 handles networking (WiFi, UDP) and Core 1 drives the WS2812 LEDs using the PIO (Programmable I/O) state machine. The PIO is unique to the RP2040 -- it generates the precise timing signals WS2812 strips require without tying up a CPU core or relying on bit-banging. The Pico SDK is also well-documented and straightforward to cross-compile. An ESP32 could work, but the PIO advantage and the simpler dual-core model made the Pico W a better fit.

### Why does the Pico firmware use C instead of C++?

The firmware targets a bare-metal microcontroller with 264 KB of RAM. C keeps the binary small, avoids hidden allocations from constructors and exceptions, and matches the Pico SDK's own C API. The codebase is simple enough that C's lack of abstractions is not a problem -- the firmware is essentially a state machine, a UDP listener, and a LED driver.

### How do devices stay in sync if the tempo changes?

When the beat detector identifies a tempo change, the server updates its internal tempo state and sends a new NEXT_BEAT message to every registered device with the updated `tempo_period_us` and `next_beat_time_ref`. Devices on the Pico re-enter the TEMPO_SYNCED state (a self-transition) and immediately adjust their LED timing. Because the NEXT_BEAT message includes the absolute time reference for the next beat (in server time, converted via the clock offset), all devices converge on the same beat instant regardless of when they received the update.

### Why use HTTPS for the web client instead of plain HTTP?

Modern browsers restrict several PWA features (service workers, `Add to Home Screen`, sensor APIs) to secure contexts. Since the web client is designed to be installed as a PWA on phones, HTTPS is required. The server generates a self-signed TLS certificate on first run, which is sufficient for a local network deployment.

### Can I add my own LED patterns?

Yes. Patterns are simple C functions in `src/ws2812/programs/` in the [beatled-pico](https://github.com/oost/beatled-pico) repo. Each pattern receives the current beat position (0-255, representing progress through the current beat) and a beat counter. You write pixel colors into a buffer, and the framework handles timing and WS2812 output. See the existing patterns (snakes, sparkles, fade) for examples. After adding a new pattern, assign it an ID in the program table and it becomes selectable from the web dashboard.

### How many devices can run simultaneously?

The system has been tested with up to a dozen Pico W devices on a single WiFi network. The limiting factor is WiFi bandwidth and airtime: each NEXT_BEAT message is 19 bytes sent via unicast, so even at 180 BPM (3 beats/second) the traffic per device is negligible. The practical limit is more about WiFi congestion in crowded environments. The server tracks connected devices and automatically expires any that stop responding within 30 seconds.
