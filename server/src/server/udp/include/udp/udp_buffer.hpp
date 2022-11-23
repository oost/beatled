#ifndef UDP__UDP_BUFFER_H
#define UDP__UDP_BUFFER_H

#include <array>
#include <asio.hpp>
#include <cstdint>
#include <exception>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <iterator>
#include <memory>
#include <span>

#include "udp_buffer.hpp"

namespace server {

class UDPBuffer {
public:
  constexpr static int BUFFER_SIZE = 1024;
  typedef std::array<uint8_t, BUFFER_SIZE> buffer_t;

  UDPBuffer() = default;
  UDPBuffer(const asio::ip::udp::endpoint &remote_endpoint)
      : remote_endpoint_{remote_endpoint} {}

  const buffer_t &data() const { return data_; }
  std::size_t size() const { return size_; }
  uint8_t type() const;
  friend std::ostream &operator<<(std::ostream &os, const UDPBuffer &buffer);

  asio::ip::udp::endpoint &remote_endpoint() { return remote_endpoint_; };

  [[nodiscard]] auto begin() const noexcept { return data_.begin(); }
  [[nodiscard]] auto end() const noexcept {
    return std::next(data_.begin(), size_);
  }

protected:
  buffer_t data_;
  std::size_t size_ = 0;
  asio::ip::udp::endpoint remote_endpoint_;
};

class UDPResponseBuffer : public UDPBuffer {
public:
  using Ptr = std::unique_ptr<UDPResponseBuffer>;
  UDPResponseBuffer() = default;

  UDPResponseBuffer(const asio::ip::udp::endpoint &remote_endpoint)
      : UDPBuffer{remote_endpoint} {}

  // void set_error_response(uint8_t error_code);

  // void set_hello_response(uint16_t pico_id);

  // void set_time_response(uint64_t orig_time, uint64_t recv_time,
  //                        uint64_t xmit_time);

  // void set_tempo_response(uint64_t beat_time_ref, uint32_t tempo_period_us);

protected:
  template <typename T> void set_data(const T &data);
};

class UDPErrorResponseBuffer : public UDPResponseBuffer {
public:
  UDPErrorResponseBuffer(uint8_t error_code);
};

class UDPHelloResponseBuffer : public UDPResponseBuffer {
public:
  UDPHelloResponseBuffer(uint16_t pico_id);
};

class UDPTimeResponseBuffer : public UDPResponseBuffer {
public:
  UDPTimeResponseBuffer(uint64_t orig_time, uint64_t recv_time,
                        uint64_t xmit_time);
};

class UDPTempoResponseBuffer : public UDPResponseBuffer {
public:
  UDPTempoResponseBuffer(uint64_t beat_time_ref, uint32_t tempo_period_us);
};

class UDPRequestBuffer : public UDPBuffer {
public:
  using Ptr = std::unique_ptr<UDPRequestBuffer>;
  using const_Ptr = std::unique_ptr<const UDPRequestBuffer>;

  buffer_t &data() { return data_; }
  void setSize(std::size_t size) {
    if (size > BUFFER_SIZE) {
      throw std::overflow_error("size > buffer size");
    }
    size_ = size;
  }
};

} // namespace server

// // template <>
// // struct fmt::formatter<server::UDPResponseBuffer> : ostream_formatter {};
// template <typename T>
// struct fmt::formatter<
//     T, std::enable_if_t<std::is_base_of<server::UDPBuffer, T>::value, char>>
//     : fmt::formatter<string_view> {

//   // Formats the point p using the parsed format specification (presentation)
//   // stored in this formatter.
//   template <typename FormatContext>
//   auto format(const T &buffer, FormatContext &ctx) const
//       -> decltype(ctx.out()) {
//     // ctx.out() is an output iterator to write to.
//     auto begin = buffer.data().begin();
//     auto end = std::next(begin, buffer.size());

//     return fmt::format_to(ctx.out(), "{::x}", fmt::join(begin, end, ":"));
//   }
// };

// template <>
// struct fmt::formatter<server::UDPResponseBuffer> : ostream_formatter {};

// template <>
// struct fmt::formatter<server::UDPResponseBuffer> : formatter<std::string> {
//   // parse is inherited from formatter<string_view>.
//   template <typename FormatContext>
//   auto format(server::UDPResponseBuffer buf, FormatContext &ctx) const {
//     auto it = buf.data().begin();
//     auto end = std::next(buf.data().begin(), buf.size());
//     return formatter<std::string>::format(fmt::join(it, end, ":"), ctx);
//   }
// };

#endif // UDP__UDP_BUFFER_H