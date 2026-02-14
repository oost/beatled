#ifndef SERVER__SRC__BEAT_DETECTOR__BEAT_DETECTOR_IMPL__H_
#define SERVER__SRC__BEAT_DETECTOR__BEAT_DETECTOR_IMPL__H_

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
#include "core/clock.hpp"

namespace beatled::detector {

using namespace std::chrono_literals;

using beatled::core::Clock;

class BeatDetector::Impl {
public:
  Impl(uint32_t sample_rate, std::size_t audio_buffer_size,
       beat_detector_cb_t beat_callback, beat_detector_cb_t next_beat_callback)
      : sample_rate_{sample_rate}, audio_buffer_size_{audio_buffer_size},
        beat_callback_{beat_callback},
        next_beat_callback_{next_beat_callback}, beat_count_{0}

  {
    audio_buffer_pool_ =
        std::make_unique<AudioBufferPool>(audio_buffer_size_, sample_rate_);
    if (beat_callback_) {
      beat_tracker_.set_beat_callback(
          [&](double tempo, double estimated_tempo) {
            SPDLOG_INFO("Beat: buffer start time {}, current_time {}, diff {}",
                        this->audio_buffer_->start_time(), Clock::time_us_64(),
                        (int64_t)(this->audio_buffer_->start_time()) -
                            (int64_t)(Clock::time_us_64()));
            beat_count_++;
            beat_callback_(this->audio_buffer_->start_time(), tempo,
                           estimated_tempo, beat_count_);
          });
    }

    if (next_beat_callback_) {
      beat_tracker_.set_next_beat_callback(
          [&](uint64_t delay, double tempo, double estimated_tempo) {
            SPDLOG_INFO(
                "Next beat: buffer start time {}, mid {}, delay {}, next beat "
                "time {}, current_time {}, diff "
                "{}",
                this->audio_buffer_->start_time(),
                this->audio_buffer_->start_time(), delay,
                this->audio_buffer_->start_time() + delay, Clock::time_us_64(),
                (int64_t)this->audio_buffer_->start_time() + (int64_t)delay -
                    (int64_t)Clock::time_us_64());

            next_beat_callback_(this->audio_buffer_->start_time() + delay,
                                tempo, estimated_tempo, beat_count_ + 1);
          });
    }
  }

  void do_detect_tempo();

  std::future<void> bd_thread_future_;
  std::atomic_bool stop_requested_;
  std::atomic_bool is_running_ = false;

  uint32_t sample_rate_;
  std::size_t audio_buffer_size_;

  beat_detector_cb_t beat_callback_ = [](uint64_t next_beat, double tempo,
                                         double estimated_tempo,
                                         uint32_t beat_count) {};
  beat_detector_cb_t next_beat_callback_ = [](uint64_t next_beat, double tempo,
                                              double estimated_tempo,
                                              uint32_t beat_count) {};

  std::unique_ptr<AudioBufferPool> audio_buffer_pool_;
  btrack::BTrack beat_tracker_;

  AudioBuffer::Ptr audio_buffer_;

  uint32_t beat_count_;
};

} // namespace beatled::detector
#endif // SERVER__SRC__BEAT_DETECTOR__BEAT_DETECTOR_IMPL__H_