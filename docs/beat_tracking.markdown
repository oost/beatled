---
title: Beat Tracking
layout: default
nav_order: 6
---

# Beat Tracking

In order to synchronize the LEDs with the music, we are running real-time beat tracking in the server.

![diagram](/beatled/assets/images/flow.svg){: width="300" }

At a high level, the flow of the signal is:

1. Record audio samples with a US microphone (controlled by PortAudio)
2. PortAudio then calls our callback with a buffer of 512 frames. Note: this call back is a realtime callback and very sensitive to any delays. We thus run it in it's dedicated thread.
3. We then push the buffer to a synchronized queue.
4. The Beat Detector, which runs in a separate thread, pops the buffer and runs another cycle of the beat tracking algorithm. We are relying on Adam Stark's [BTrack](https://github.com/adamstark/BTrack) library. We found it to be one of the most efficient realtime trackers. Given its lack of C++ support and separation of concerns, we rewrote the library in the spirit of [Essentia](https://essentia.upf.edu/documentation.html), with small bricks of composable elements, chained in a pipeline.
5. The Beat Detector then send the update tempo and beat timing to the Broadcast which broadcasts it of UDP to all Pico boards on the network.

![diagram](/beatled/assets/images/comms.svg){: width="550" }
