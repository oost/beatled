#ifndef BEAT_DETECTOR_BEAT_DETECTOR_H
#define BEAT_DETECTOR_BEAT_DETECTOR_H

#include <asio.hpp>
#include <atomic>
#include <experimental/propagate_const>

#include "core/interfaces/service_controller.hpp"

using beatled::core::ServiceControllerInterface;

namespace beatled::detector {

/**
 * @brief Interface to BeatDetector
 */
class BeatDetector : public ServiceControllerInterface {
public:
  using beat_detector_cb_t =
      std::function<void(uint64_t, double, double, uint32_t)>;

  BeatDetector(const std::string &id, uint32_t sample_rate,
               std::size_t audio_buffer_size,
               beat_detector_cb_t beat_callback = nullptr,
               beat_detector_cb_t next_beat_callback = nullptr);
  ~BeatDetector();

  /**
   * @brief Start beat detector service
   */
  void start_sync() override;

  /**
   * @brief Stop beat detector algorithm, async
   */
  void stop_sync() override;

  /**
   * @brief Stop beat detector algorithm, sync
   */
  void stop_blocking();

private:
  const char *service_name() const override { return SERVICE_NAME; }
  const char *SERVICE_NAME = "Beat Detector";

  class Impl;
  // const-forwarding pointer wrapper
  // unique-ownership opaque pointer
  // to the forward-declared implementation class
  std::experimental::propagate_const<std::unique_ptr<Impl>> pImpl;
};

} // namespace beatled::detector
#endif // BEAT_DETECTOR_BEAT_DETECTOR_H