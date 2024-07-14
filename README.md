# Beatled

## Prerequisites

- A Raspberry Pi 4 or 5
- One or more Raspberry Pico
- A Unix/Linux based machine (not strictly necessary but it may be easier to use your laptop to cross compile the code)

## Repos

- This repo contains the C++ server code and the React frontend.
- The [Beatled Pico](https://github.com/oost/beatled-pico) repo contains the Pico C code to be flashed on your Pico devices.
- The [Beatled Beat Tracker](https://github.com/oost/beatled-beat-tracker) repo contains a fork from [BTrack](https://github.com/adamstark/BTrack) that is used for live beat tracking.

## Cross Compilation

In this step, we cross-compile the various server for the Raspberry Pi. We also build the React front and package it in a tarball.

1. Download submodule dependencies.

   ```
   git sumbodule update
   ```

2. Build builder image:

   ```
   utils/build-docker-builder.sh
   ```

3. Build Raspberry Pi executable:

   ```
   utils/build-beatled-server.sh
   ```

4. Copy your files to the raspberry pi:

   ```
   utils/copy-tar-files.sh ${RPI_USERNAME} ${RPI_HOST}
   ```

## Requirements

- On MacOS
  - `brew install pkg-config cmake libtool automake autoconf autoconf-archive`
