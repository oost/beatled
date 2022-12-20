#include <BTrack.h>
#include <chrono>
#include <fmt/format.h>
#include <future>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sys/time.h>

#include "./audio_exception.hpp"
#include "./audio_input.hpp"
#include "beat_detector/beat_detector.hpp"
#include "state_manager/clock.hpp"
#include "state_manager/state_manager.hpp"

using namespace beat_detector;
using namespace std::chrono_literals;

BeatDetector::BeatDetector(StateManager &state_manager, uint32_t sample_rate)
    : state_manager_{state_manager}, sample_rate_{sample_rate} {}

void BeatDetector::request_stop() {
  SPDLOG_INFO("Requesting Beat Detector to stop");
  stop_requested_ = true;
}

void BeatDetector::stop_blocking() {
  request_stop();
  SPDLOG_INFO("Waiting for Beat Detector to stop");
  if (bd_thread_future_.valid()) {
    bd_thread_future_.wait();
    bd_thread_future_.get();
  }
}

bool BeatDetector::is_running() { return is_running_; }

void BeatDetector::run() {
  SPDLOG_INFO("Starting Beat Detector");

  if (!bd_thread_future_.valid() ||
      bd_thread_future_.wait_for(0s) == std::future_status::ready) {
    SPDLOG_INFO("Starting thread");
    stop_requested_.store(false);
    bd_thread_future_ = std::async(
        std::launch::async, [this]() -> void { return do_detect_tempo(); });
  } else {
    SPDLOG_INFO("Beat detector is already running");
  }
}

void BeatDetector::do_detect_tempo() {
  AudioBufferPool audio_buffer_pool{constants::audio_buffer_size};
  BTrack beat_tracker;

  // Use frame_rate = 0 to let the OS choose the frame rate (potentially
  // dynamcially)
  AudioInput audio_input(audio_buffer_pool, sample_rate_,
                         constants::audio_buffer_size);

  if (!audio_input.open()) {
    throw AudioInputException("Couldn't open device.");
  }

  if (!audio_input.start()) {
    throw AudioInputException("Couldn't start stream.");
  }

  SPDLOG_INFO("Audio input active: {}", audio_input.is_active());

  is_running_ = true;
  while (true) {
    AudioBuffer::Ptr buffer = audio_buffer_pool.dequeue_blocking();

    auto hop_data = buffer->data();
    beat_tracker.processAudioFrame(hop_data.data());

    if (beat_tracker.beatDueInCurrentFrame()) {
      uint64_t timeref = Clock::time_since_epoch_us();

      SPDLOG_DEBUG("Beat: tempo: {}, timestamp: {} ",
                   beat_tracker.getCurrentTempoEstimate(), timeref);

      state_manager_.update_tempo(beat_tracker.getCurrentTempoEstimate(),
                                  timeref);
    }

    audio_buffer_pool.release_buffer(std::move(buffer));

    if (stop_requested_.load()) {
      SPDLOG_INFO("Stopping thread");
      audio_input.stop();

      break;
    }
  }
  is_running_ = false;
  SPDLOG_INFO("Exiting beat detector loop");
}
