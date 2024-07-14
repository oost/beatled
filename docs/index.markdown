---
title: Home
layout: home
nav_order: 1
permalink: /
---

Welcome to the `Beatled` project.

## Prerequisites

- A Raspberry Pi 4 or 5
- One or more Raspberry Pico W
- A Unix/Linux based machine with Docker (not strictly necessary but it may be easier to use your laptop to cross compile the code)

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

---

[^1]: [It can take up to 10 minutes for changes to your site to publish after you push the changes to GitHub](https://docs.github.com/en/pages/setting-up-a-github-pages-site-with-jekyll/creating-a-github-pages-site-with-jekyll#creating-your-site).

[Just the Docs]: https://just-the-docs.github.io/just-the-docs/
[GitHub Pages]: https://docs.github.com/en/pages
[README]: https://github.com/just-the-docs/just-the-docs-template/blob/main/README.md
[Jekyll]: https://jekyllrb.com
[GitHub Pages / Actions workflow]: https://github.blog/changelog/2022-07-27-github-pages-custom-github-actions-workflows-beta/
[use this template]: https://github.com/just-the-docs/just-the-docs-template/generate
