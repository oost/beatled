---
title: Beatled Pico
layout: default
parent: Components
nav_order: 2
---

# Beatled Pico

Source code: [github.com/oost/beatled-pico](https://github.com/oost/beatled-pico)

Beatled Pico is embedded C firmware designed to run on a **Raspberry Pi Pico W** microcontroller. The Pico is mounted on wearables -- sewn into jackets, wrapped around hats, or embedded in costumes -- alongside a WS2812 LED strip and a small battery pack.

The firmware connects to the Beatled server over WiFi and receives tempo data and control commands via a binary UDP protocol. Using this timing information, it drives the LED strip with beat-synchronized lighting patterns, keeping all devices on the network visually in sync.

The project links against the [Pico SDK](https://github.com/raspberrypi/pico-sdk) and compiles to a **bare-metal UF2 executable** that is flashed directly onto the Pico W -- there is no operating system. The dual-core RP2040 chip is split by design: Core 0 handles WiFi networking and the synchronization state machine, while Core 1 drives the WS2812 LEDs using the chip's PIO (Programmable I/O) hardware for cycle-accurate signal timing.

For development without physical hardware, the project includes a **macOS posix port** that builds a native ARM64 executable. It replaces the Pico SDK's hardware APIs with POSIX equivalents (pthreads, UDP sockets) and renders a 3D simulation of an LED ring using Metal shaders, so you can iterate on patterns and debug networking on your Mac.

## Requirements

- Raspberry Pico W
- AAA Battery holder ([link to Adafruit](https://www.adafruit.com/product/727))
- Wire ([link to Adafruit](https://www.adafruit.com/product/2517))
- WS2812 LED Strip ([link to Sparkfun](https://www.sparkfun.com/products/retired/15206))
- A hat or clothes to mount it on ([example](https://www.amazon.com/dp/B09YCHTGWZ?psc=1&ref=ppx_yo2ov_dt_b_product_details))

## Getting Started

Clone the repo and initialise submodules (includes the Pico SDK):

```bash
git clone https://github.com/oost/beatled-pico.git
cd beatled-pico
git submodule update --init
```

## Building for Pico W (UF2)

This cross-compiles the firmware into a `.uf2` binary that you flash directly onto the Pico W.

### Dependencies

```bash
brew install cmake
brew install --cask gcc-arm-embedded   # ARM cross-compiler
brew install openocd                    # on-chip debugger (optional)
brew install minicom                    # serial monitor (optional)
```

([Full Pico SDK setup instructions](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf))

### Build

Set the environment variables for your network and server, then configure and build:

```bash
export PICO_TOOLCHAIN_PATH="/Applications/ArmGNUToolchain/12.2.rel1/arm-none-eabi"
export BEATLED_SERVER_NAME="raspberrypi1.local"
export WIFI_SSID="your-wifi"
export WIFI_PASSWORD="your-password"

cmake -B build-pico \
  -DPORT=pico \
  -DPICO_BOARD=pico_w \
  -DPICO_SDK_PATH=lib/pico-sdk

cmake --build build-pico
```

The output binary is at `build-pico/src/pico_w_beatled.uf2`.

### Flashing

Hold the BOOTSEL button on the Pico W while plugging it in via USB. It mounts as a USB drive. Copy the UF2 file onto it:

```bash
cp build-pico/src/pico_w_beatled.uf2 /Volumes/RPI-RP2/
```

The Pico reboots automatically and starts running the firmware.

### Serial Monitor

To view logs over USB serial:

```bash
minicom -b 115200 -D /dev/tty.usbmodem*
```

---

## Building the POSIX Port (macOS)

The POSIX port builds a native macOS executable that replaces Pico hardware APIs with POSIX equivalents (pthreads, UDP sockets) and renders a 3D LED ring simulation using Metal shaders.

![Image](/beatled/assets/images/simulator-new.gif)

### Dependencies

```bash
brew install cmake
```

[vcpkg](https://vcpkg.io/) must be installed and `VCPKG_ROOT` set in your environment.

### Build

```bash
cmake -B build \
  -DPORT=posix \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build
```

### Run

```bash
./build/src/pico_w_beatled.app/Contents/MacOS/pico_w_beatled
```

Or using the project utility script from the `beatled` repo:

```bash
utils/beatled.sh pico
```

By default, the POSIX port connects to `localhost`. Set `BEATLED_SERVER_NAME` before configuring to override.

### Running Tests

```bash
cmake --build build
./build/tests/posix/integration/test_integration
./build/tests/posix/command/test_command
./build/tests/posix/clock/test_clock
./build/tests/posix/queue/test_queue
```

### Debugging in VS Code

The project ships with a ready-to-use VS Code debug configuration for the POSIX port.

1. Open the `beatled-pico` folder in VS Code
2. Make sure `.vscode/settings.json` has the POSIX port selected (this is the default):

   ```json
   "cmake.configureSettings": {
     "PORT": "posix",
     "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
   }
   ```

3. Configure and build via the CMake extension (or the terminal commands above)
4. Set `pico_w_beatled` as the CMake launch target (`CMake: Set Launch Target`)
5. Press **F5** or select the **(lldb) Launch** configuration

   This launches the POSIX executable under LLDB with full source-level debugging -- breakpoints, variable inspection, and stepping all work as expected.

## LED Patterns

The firmware ships with 8 built-in patterns:

| ID  | Name       | Description                              |
| --- | ---------- | ---------------------------------------- |
| 0   | Snakes     | Animated snake trails along the strip    |
| 1   | Random     | Random pixel colors                      |
| 2   | Sparkles   | Twinkling sparkle effect                 |
| 3   | Greys      | Greyscale gradient                       |
| 4   | Drops      | Raindrop-style pulses                    |
| 5   | Solid      | Solid brightness driven by beat fraction |
| 6   | Fade       | Grey fade synced to beat                 |
| 7   | Fade Color | Color fade synced to beat                |

Patterns are simple C functions that receive the beat position (0-255) and beat count. New patterns can be added in `src/ws2812/programs/`.

## Wiring Diagram

![Image](/beatled/assets/images/circuit.png)

[Circuit](https://crcit.net/c/e7308720163247329f2c2b24adeabc57)
