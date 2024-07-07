#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "udp/udp_buffer.hpp"

namespace beatled::server {

std::ostream &operator<<(std::ostream &os, const DataBuffer &buffer) {
  using namespace std;

  for (int i = 0; i < buffer.size(); i++) {
    os << hex << setfill('0') << setw(2) << static_cast<int>(buffer.data()[i]);
    if (i < buffer.size() - 1) {
      os << ":";
    }
  }

  return os;
}

uint8_t DataBuffer::type() const {
  if (size_ == 0) {
    throw std::range_error("Size of buffer is 0");
  }
  return data_[0];
}
} // namespace beatled::server
