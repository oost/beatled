#ifndef UDP__UDP_BUFFER_H
#define UDP__UDP_BUFFER_H

#include <array>
#include <cstdint>
#include <memory>

#include "udp_buffer.hpp"

namespace server {

class UDPBuffer {
public:
  constexpr static int BUFFER_SIZE = 1024;
  typedef std::array<uint8_t, BUFFER_SIZE> buffer_t;

  buffer_t &data() { return data_; }
  const buffer_t &data_const() const { return data_; }
  std::size_t size() const { return size_; }
  void setSize(std::size_t size) { size_ = size; }
  void print_buffer() const;
  uint8_t type() const;
  friend std::ostream &operator<<(std::ostream &os, const UDPBuffer &buffer);

private:
  buffer_t data_;
  std::size_t size_;
};

class UDPRequestBuffer : public UDPBuffer {
public:
  using Ptr = std::unique_ptr<UDPRequestBuffer>;
};
class UDPResponseBuffer : public UDPBuffer {
public:
  using Ptr = std::unique_ptr<UDPResponseBuffer>;
};
} // namespace server

#endif // UDP__UDP_BUFFER_H