// TempoBroadcaster behaviour tests. A loopback UDP socket plays the part
// of a registered controller; the io_context runs single-threaded inside
// the test via run_for, so timing assertions (the 50 ms program-push
// retry, the status-probe cadence) are deterministic enough for CI.

#include <catch2/catch_test_macros.hpp>

#include <asio.hpp>
#include <chrono>
#include <cstring>
#include <vector>

#include "beatled/network.h"
#include "beatled/protocol.h"
#include "core/clock.hpp"
#include "core/state_manager.hpp"
#include "tempo_broadcaster/tempo_broadcaster.hpp"

using asio::ip::udp;
using beatled::core::ClientStatus;
using beatled::core::Clock;
using beatled::core::StateManager;
using beatled::server::BroadcastMode;
using beatled::server::TempoBroadcaster;

namespace {

template <typename T> T parse_message(const std::vector<uint8_t> &datagram) {
  REQUIRE(datagram.size() == sizeof(T));
  T msg;
  std::memcpy(&msg, datagram.data(), sizeof(T));
  return msg;
}

// One loopback "controller": a bound UDP socket plus the ClientStatus
// registration that points the broadcaster's unicast path at it.
struct FakeController {
  explicit FakeController(asio::io_context &io)
      : socket(io, udp::endpoint(asio::ip::make_address_v4("127.0.0.1"), 0)) {
    arm();
  }

  void register_with(StateManager &sm) {
    ClientStatus::board_id_t bid{};
    bid[0] = 'T';
    auto cs = std::make_shared<ClientStatus>(bid, socket.local_endpoint().address());
    cs->last_status_time = Clock::wall_time_us_64();
    cs->endpoint = socket.local_endpoint();
    sm.register_client(cs);
  }

  void arm() {
    socket.async_receive(asio::buffer(rx_buf), [this](std::error_code ec, std::size_t n) {
      if (!ec) {
        received.emplace_back(rx_buf.begin(), rx_buf.begin() + n);
        arm();
      }
    });
  }

  udp::socket socket;
  std::array<uint8_t, 64> rx_buf{};
  std::vector<std::vector<uint8_t>> received;
};

struct Harness {
  explicit Harness(std::chrono::nanoseconds status_probe_period = std::chrono::nanoseconds{0})
      : controller(io), broadcaster("test", io, std::chrono::hours(1), status_probe_period,
                                    {"127.0.0.1", 0, BroadcastMode::Unicast}, state_manager) {
    controller.register_with(state_manager);
  }

  ~Harness() { broadcaster.stop(); }

  void run_for(std::chrono::milliseconds d) {
    io.restart();
    io.run_for(d);
  }

  asio::io_context io;
  StateManager state_manager;
  FakeController controller;
  TempoBroadcaster broadcaster;
};

} // namespace

TEST_CASE("broadcast_next_beat unicasts NEXT_BEAT with incrementing seq", "[tempo_broadcaster]") {
  Harness h;
  h.broadcaster.start();

  h.broadcaster.broadcast_next_beat(123456789ULL, 42);
  h.broadcaster.broadcast_next_beat(123956789ULL, 43);
  h.run_for(std::chrono::milliseconds(100));

  REQUIRE(h.controller.received.size() == 2);
  auto first = parse_message<beatled_message_next_beat_t>(h.controller.received[0]);
  CHECK(first.base.type == BEATLED_MESSAGE_NEXT_BEAT);
  CHECK(ntohll(first.next_beat_time_ref) == 123456789ULL);
  CHECK(ntohl(first.beat_count) == 42);
  CHECK(ntohs(first.seq) == 0);

  auto second = parse_message<beatled_message_next_beat_t>(h.controller.received[1]);
  CHECK(ntohll(second.next_beat_time_ref) == 123956789ULL);
  CHECK(ntohl(second.beat_count) == 43);
  CHECK(ntohs(second.seq) == 1);
}

TEST_CASE("push_program_now sends twice with the same seq (50 ms retry)", "[tempo_broadcaster]") {
  Harness h;
  h.state_manager.update_program_id(3);
  h.broadcaster.start();

  h.broadcaster.push_program_now();
  h.run_for(std::chrono::milliseconds(200));

  REQUIRE(h.controller.received.size() == 2);
  auto push = parse_message<beatled_message_program_t>(h.controller.received[0]);
  CHECK(push.base.type == BEATLED_MESSAGE_PROGRAM);
  CHECK(ntohs(push.program_id) == 3);
  // The retry must be byte-identical so controllers treat it as an
  // idempotent duplicate.
  CHECK(h.controller.received[0] == h.controller.received[1]);
}

TEST_CASE("program changes push instantly via the StateManager callback", "[tempo_broadcaster]") {
  Harness h;
  h.broadcaster.start();

  h.state_manager.update_program_id(7);
  h.run_for(std::chrono::milliseconds(200));

  REQUIRE(h.controller.received.size() >= 1);
  auto push = parse_message<beatled_message_program_t>(h.controller.received[0]);
  CHECK(push.base.type == BEATLED_MESSAGE_PROGRAM);
  CHECK(ntohs(push.program_id) == 7);
}

TEST_CASE("status probes fire on the configured cadence", "[tempo_broadcaster]") {
  Harness h(std::chrono::milliseconds(20));
  h.broadcaster.start();

  const uint64_t before_us = Clock::wall_time_us_64();
  h.run_for(std::chrono::milliseconds(150));

  REQUIRE(h.controller.received.size() >= 2);
  auto probe = parse_message<beatled_message_status_request_t>(h.controller.received[0]);
  CHECK(probe.base.type == BEATLED_MESSAGE_STATUS_REQUEST);
  // The echoed send time is wall-clock microseconds stamped by the server.
  CHECK(ntohll(probe.server_send_time_us) >= before_us);
  CHECK(ntohll(probe.server_send_time_us) <= Clock::wall_time_us_64());
}

TEST_CASE("a stopped broadcaster drops broadcasts", "[tempo_broadcaster]") {
  Harness h;
  h.broadcaster.start();
  h.broadcaster.stop();

  h.broadcaster.broadcast_next_beat(1, 1);
  h.broadcaster.push_program_now();
  h.run_for(std::chrono::milliseconds(100));

  CHECK(h.controller.received.empty());
}
