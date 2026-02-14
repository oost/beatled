---
title: Beatled Pico
layout: default
nav_order: 5
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

## Compilation

1. Clone the [`beatled-pico`](https://github.com/oost/beatled-pico) repo
2. Checkout submodules:

   ```
   git submodule update --init
   ```

3. Install the dependencies. For MacOS:

   ```
   brew install cmake
   brew tap ArmMbed/homebrew-formulae
   brew install gcc-arm-embedded
   brew install openocd
   brew install minicom
   ```

   Make sure vcpkg is also installed on your machine.

   ([Full instructions](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf))

4. You can now build the project from VS Code. To start with, you can build the project for MacOS. It will generate an ARM64 executable with a Metal Shaders visualization. In `.vscode/settings.json`, make sure the following line are uncommented:

   ```
    "PORT": "posix",
    "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
   ```

   ![Image](/beatled/assets/images/simulator.gif)

5. If you want to compile the Pico version, you need to:
   1. Delete the `build` folder
   2. Select the `arm-none-eabi` kit in Cmake (using the `Cmake: Select Kit` command)
   3. Rebuild the executables

### Local Simulation

You can also build and run the Pico firmware on your Mac using the posix port with a Metal-based LED visualizer:

```bash
utils/beatled.sh pico
```

## LED Patterns

The firmware ships with 8 built-in patterns:

| ID | Name | Description |
|----|------|-------------|
| 0 | Snakes | Animated snake trails along the strip |
| 1 | Random | Random pixel colors |
| 2 | Sparkles | Twinkling sparkle effect |
| 3 | Greys | Greyscale gradient |
| 4 | Drops | Raindrop-style pulses |
| 5 | Solid | Solid brightness driven by beat fraction |
| 6 | Fade | Grey fade synced to beat |
| 7 | Fade Color | Color fade synced to beat |

Patterns are simple C functions that receive the beat position (0-255) and beat count. New patterns can be added in `src/ws2812/programs/`.

## Wiring Diagram

![Image](/beatled/assets/images/circuit.png)

[Circuit](https://crcit.net/c/e7308720163247329f2c2b24adeabc57)
