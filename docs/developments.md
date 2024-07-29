---
title: Futrher Developments
layout: default
nav_order: 6
---

# Beat Tracking Frameworks

## "Traditional" Signals-based Beat Traking

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
