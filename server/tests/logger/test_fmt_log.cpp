#include <catch2/catch_test_macros.hpp>
#include <fmt/ranges.h>
#include <udp/udp_buffer.hpp>

TEST_CASE("Formatting UDP buffers to hex with fmt", "[fmt]") {
  server::UDPRequestBuffer buf;
  buf.data()[0] = 11;
  buf.data()[1] = 54;
  buf.setSize(1);
  std::string s1 = fmt::format("{::x}", buf);
  REQUIRE(s1 == "[b]");
  buf.setSize(2);
  std::string s2 = fmt::format("{::x}", buf);
  REQUIRE(s2 == "[b, 36]");
}