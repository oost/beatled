---
title: Beatled Pico
layout: default
nav_order: 5
---

# Beatled Pico

Source code: [github.com/oost/beatled-pico](https://github.com/oost/beatled-pico)

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
