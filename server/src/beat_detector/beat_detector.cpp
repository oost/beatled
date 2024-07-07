#include <beat_tracker.hpp>
#include <chrono>
#include <fmt/format.h>
#include <future>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sys/time.h>

#include "audio/audio_exception.hpp"
#include "audio/audio_input.hpp"
#include "beat_detector/beat_detector.hpp"
#include "beat_detector_impl.h"
#include "core/clock.hpp"

namespace beatled::detector {

using namespace std::chrono_literals;

using beatled::core::Clock;

BeatDetector::BeatDetector(const std::string &id, uint32_t sample_rate,
                           std::size_t audio_buffer_size,
                           beat_detector_cb_t beat_callback,
                           beat_detector_cb_t next_beat_callback)
    : pImpl{std::make_unique<Impl>(sample_rate, audio_buffer_size,
                                   beat_callback, next_beat_callback)},
      ServiceControllerInterface{id} {}

BeatDetector::~BeatDetector() {}

void BeatDetector::stop_sync() {
  SPDLOG_INFO("Requesting Beat Detector to stop");
  pImpl->stop_requested_ = true;
  if (pImpl->bd_thread_future_.valid()) {
    pImpl->bd_thread_future_.wait();
    pImpl->bd_thread_future_.get();
  }
}

void BeatDetector::stop_blocking() {
  stop();
  SPDLOG_INFO("Waiting for Beat Detector to stop");
  if (pImpl->bd_thread_future_.valid()) {
    pImpl->bd_thread_future_.wait();
    pImpl->bd_thread_future_.get();
  }
}

void BeatDetector::start_sync() {
  if (!pImpl->bd_thread_future_.valid() ||
      pImpl->bd_thread_future_.wait_for(0s) == std::future_status::ready) {
    SPDLOG_INFO("Starting thread");
    pImpl->stop_requested_.store(false);
    pImpl->bd_thread_future_ = std::async(std::launch::async, [this]() -> void {
      return pImpl->do_detect_tempo();
    });
  } else {
    SPDLOG_INFO("Beat detector is already running");
  }
}

void BeatDetector::Impl::do_detect_tempo() {
  // Use frame_rate = 0 to let the OS choose the frame rate (potentially
  // dynamcially)
  AudioInput audio_input(audio_buffer_pool_.get(), sample_rate_, 512);

  if (!audio_input.open()) {
    throw AudioInputException("Couldn't open device.");
  }

  if (!audio_input.start()) {
    throw AudioInputException("Couldn't start stream.");
  }

  SPDLOG_INFO("Audio input active: {}", audio_input.is_active());

  beat_tracker_.set_sampling_rate(audio_input.effective_sample_rate());

  is_running_ = true;
  uint64_t previous_buffer_time = 0;
  while (true) {
    audio_buffer_ = audio_buffer_pool_->dequeue_blocking();

    if (!audio_buffer_) {
      SPDLOG_INFO("Stopping thread");
      audio_input.stop();
      break;
    }
    auto diff = static_cast<int64_t>(previous_buffer_time) -
                static_cast<int64_t>(audio_buffer_->start_time());
    if (abs(diff) < 2) {
      SPDLOG_INFO("Got same buffer twice (diff {})... Hmmm. Dropping it.",
                  diff);
      audio_buffer_pool_->release_buffer(std::move(audio_buffer_));
      continue;
    }

    auto hop_data = audio_buffer_->data();
    beat_tracker_.process_audio_frame(hop_data);

    // SPDLOG_INFO("Buffer time {}, previous {}, diff {}",
    //             audio_buffer_->start_time(), previous_buffer_time,
    //             static_cast<int64_t>(audio_buffer_->start_time()) -
    //                 static_cast<int64_t>(previous_buffer_time));
    previous_buffer_time = audio_buffer_->start_time();

    audio_buffer_pool_->release_buffer(std::move(audio_buffer_));

    if (stop_requested_.load()) {
      SPDLOG_INFO("Stopping thread");
      audio_input.stop();

      break;
    }
  }

  audio_input.wait();
  is_running_ = false;
  SPDLOG_INFO("Exiting beat detector loop");
}

} // namespace beatled::detector