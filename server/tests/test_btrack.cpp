#include <cmath>
#include <iostream>

#include "beat_tracker.hpp"
#include "transformers/buffers/circular_buffer.h"

void test_circular();
void test_btrack();

int main() {
  test_circular();
  test_btrack();

  return 0;
}

void test_circular() {
  btrack::CircularBuffer<double> buffer;

  buffer.resize(10);

  buffer.append(10);
  buffer.append(8);
  buffer.append(10);
  buffer.append(8);
  buffer.append(10);
  buffer.append(8);
  buffer.append(10);
  buffer.append(8);

  for (int i = 0; i < 10; i++) {
    std::cout << buffer[i] << std::endl;
  }
}

void test_btrack() {
  btrack::OnsetDetectionFunction odf(512, 1024);

  // BTrack b(512);

  // double *frame = new double[1024];
  // for (int i = 0; i < 1024; i++) {
  //   frame[i] = std::sin(i / M_PI / 100);
  // }

  // b.processAudioFrame(frame);

  // if (b.beatDueInCurrentFrame()) {
  //   // do something on the beat
  // }

  // b.processOnsetDetectionFunctionSample(newSample);

  // if (b.beatDueInCurrentFrame()) {
  //   // do something on the beat
  // }
}