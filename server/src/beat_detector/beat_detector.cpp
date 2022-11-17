#include <BTrack.h>
#include <chrono>
#include <fmt/format.h>
#include <future>
#include <iostream>
#include <sys/time.h>

#include "audio_exception.hpp"
#include "audio_input.hpp"
#include "beat_detector/beat_detector.hpp"
#include "state_manager/state_manager.hpp"

using namespace beat_detector;
using namespace std::chrono_literals;

BeatDetector::BeatDetector(StateManager &state_manager, uint32_t sample_rate)
    : state_manager_{state_manager}, sample_rate_{sample_rate} {}

void BeatDetector::request_stop() { stop_requested_ = true; }

void BeatDetector::run() {
  std::cout << "Starting Beat Detector" << std::endl;

  if (!bd_thread_future_.valid() ||
      bd_thread_future_.wait_for(0s) == std::future_status::ready) {
    std::cout << "Starting thread" << std::endl;
    stop_requested_.store(false);
    bd_thread_future_ = std::async(
        std::launch::async, [this]() -> void { return do_detect_tempo(); });
  } else {
    std::cout << "Beat detector is already running" << std::endl;
  }
}

void BeatDetector::do_detect_tempo() {
  AudioBufferPool audio_buffer_pool{constants::audio_buffer_size};
  BTrack beat_tracker;

  // Use frame_rate = 0 to let the OS choose the frame rate (potentially
  // dynamcially)
  AudioInput audio_input(audio_buffer_pool, sample_rate_,
                         constants::audio_buffer_size);

  // AudioInput audio_input(audio_buffer_pool, sample_rate_,
  // frames_per_buffer_);

  if (!audio_input.open()) {
    throw AudioInputException("Couldn't open device.");
  }

  if (!audio_input.start()) {
    throw AudioInputException("Couldn't start stream.");
  }

  std::cout << "Audio input active: " << audio_input.is_active() << std::endl;

  // int idx = 0;
  // const int cycle_per_second =
  //     static_cast<int>(sample_rate_) / constants::audio_buffer_size;

  while (true) {
    AudioBuffer::Ptr buffer = audio_buffer_pool.dequeue_blocking();

    auto hop_data = buffer->data();
    beat_tracker.processAudioFrame(hop_data.data());

    if (beat_tracker.beatDueInCurrentFrame()) {
      // do something on the beat
      // std::timespec ts = buffer->start_time().timespec();
      // char buf[100];

      // std::strftime(buf, sizeof buf, "%D %T", std::gmtime(&ts.tv_sec));
      std::cout << fmt::format("\nBeat (tempo: {}, timestamp: )--- ",
                               beat_tracker.getCurrentTempoEstimate())
                // << "Current time: " << buf << '.' << ts.tv_nsec << "
                // UTC\n"
                << std::endl;
      state_manager_.update_tempo(
          beat_tracker.getCurrentTempoEstimate(),
          duration_cast<std::chrono::microseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count());

    } else {
      std::cout << ".";
    }
    // const audio_buffer_data_t &buffer_data = buffer->data();
    // for (int i = 0; i < elements_to_copy; i++) {
    //   audio_data.push_back(buffer_data[i]);
    // }
    audio_buffer_pool.release_buffer(std::move(buffer));

    // idx++;
    // if (idx % cycle_per_second == 0) {
    //   std::cout << fmt::format("Recorded {} seconds",
    //                            idx * constants::audio_buffer_size /
    //                                sample_rate_)
    //             << std::endl;
    // }
    if (stop_requested_.load()) {
      std::cout << "Stopping thread" << std::endl;
      audio_input.stop();

      break;
    }
  }
  std::cout << "Exiting loop" << std::endl;
}
