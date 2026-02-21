---
title: Further Developments
layout: default
nav_order: 8
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
