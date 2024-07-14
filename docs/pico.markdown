---
title: Beatled Pico
layout: default
nav_order: 3
---

# Beatled Pico

## Requirements

- Raspberry Pico W
- AAA Battery holder ([link to Adafruit](https://www.adafruit.com/product/727))
- Wire ([link to Adafruit](https://www.adafruit.com/product/2517))
- WS2812 LED Strip ([link to Sparkfun](https://www.sparkfun.com/products/retired/15206))
- A hat to mount it on ([example](https://www.amazon.com/dp/B09YCHTGWZ?psc=1&ref=ppx_yo2ov_dt_b_product_details))

## Compilation

1. Checkout the `beatled-pico` repo
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

   Make sure vcpkg is also intalled on your machine.

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

## Wiring Diagram

    ![Image](/beatled/assets/images/circuit.png)

    [Circuit](https://crcit.net/c/e7308720163247329f2c2b24adeabc57)
