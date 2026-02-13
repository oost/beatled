---
title: Beatled Server
layout: default
nav_order: 4
---

# Beatled Client

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
