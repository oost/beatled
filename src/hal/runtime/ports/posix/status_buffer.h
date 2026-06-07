#ifndef SRC__HAL__RUNTIME__PORTS__POSIX__STATUS_BUFFER__H_
#define SRC__HAL__RUNTIME__PORTS__POSIX__STATUS_BUFFER__H_

#include <cstdint>
#include <memory>

struct StatusData {
  uint8_t state;
  bool connected;
  uint16_t program_id;
  uint32_t tempo_period_us;
  uint32_t beat_count;
  int64_t time_offset;
};

class StatusBuffer {
public:
  using Ptr = std::shared_ptr<StatusBuffer>;

  StatusBuffer() = default;
  StatusBuffer(const StatusData &d) : data_(d) {}

  static Ptr load_instance();
  static void update(const StatusData &d);

  const StatusData &data() const { return data_; }

private:
  StatusData data_{};
};

#endif // SRC__HAL__RUNTIME__PORTS__POSIX__STATUS_BUFFER__H_
