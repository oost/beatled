#include <arpa/inet.h>
#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "beatled/network.h"
#include "beatled/protocol.h"
#include "core/state_manager.hpp"
#include "udp/udp_buffer.hpp"

// UDPRequestHandler header is not in a public include dir, include directly.
#include "../../src/server/udp_server/udp_request_handler.hpp"

using namespace beatled::server;
using beatled::core::ClientStatus;
using beatled::core::StateManager;

namespace {

// Helper: create a UDPRequestBuffer with a message payload and remote endpoint.
UDPRequestBuffer make_request(const void *msg, size_t size, const char *ip = "10.0.0.1") {
  asio::ip::udp::endpoint ep(asio::ip::make_address(ip), 9000);
  UDPRequestBuffer buf(ep);
  std::memcpy(buf.data().data(), msg, size);
  buf.setSize(size);
  return buf;
}

} // namespace

TEST_CASE("UDPRequestHandler dispatch", "[udp][handler]") {
  StateManager sm;

  SECTION("Empty buffer returns NO_DATA error") {
    UDPRequestBuffer buf;
    UDPRequestHandler handler(&buf, sm);

    auto resp = handler.response();
    REQUIRE(resp->type() == BEATLED_MESSAGE_ERROR);

    const auto *err = reinterpret_cast<const beatled_message_error_t *>(&resp->data());
    REQUIRE(err->error_code == BEATLED_ERROR_NO_DATA);
  }

  SECTION("Unknown message type returns UNKNOWN_MESSAGE_TYPE error") {
    uint8_t bad_type = 0xFF;
    auto buf = make_request(&bad_type, 1);
    UDPRequestHandler handler(&buf, sm);

    auto resp = handler.response();
    REQUIRE(resp->type() == BEATLED_MESSAGE_ERROR);

    const auto *err = reinterpret_cast<const beatled_message_error_t *>(&resp->data());
    REQUIRE(err->error_code == BEATLED_ERROR_UNKNOWN_MESSAGE_TYPE);
  }
}

TEST_CASE("UDPRequestHandler hello request", "[udp][handler]") {
  StateManager sm;

  SECTION("Valid hello request registers client and returns response") {
    beatled_message_hello_request_t hello_req{};
    hello_req.base.type = BEATLED_MESSAGE_HELLO_REQUEST;
    std::memcpy(hello_req.board_id, "ABCD1234ABCD1234", sizeof(hello_req.board_id));
    std::memcpy(hello_req.port_name, "pico-freertos", sizeof("pico-freertos"));
    std::memcpy(hello_req.git_sha, "abcd123-dirty", sizeof("abcd123-dirty"));
    const uint64_t kBuildTimeUs = 1700000000000000ULL;
    hello_req.build_time_us = htonll(kBuildTimeUs);

    auto buf = make_request(&hello_req, sizeof(hello_req));
    UDPRequestHandler handler(&buf, sm);

    auto resp = handler.response();
    REQUIRE(resp->type() == BEATLED_MESSAGE_HELLO_RESPONSE);
    REQUIRE(resp->size() == sizeof(beatled_message_hello_response_t));

    // Client should now be registered and carry the v3 firmware fields.
    auto client = sm.client_status(asio::ip::make_address("10.0.0.1"));
    REQUIRE(client != nullptr);
    REQUIRE(client->port_name == "pico-freertos");
    REQUIRE(client->git_sha == "abcd123-dirty");
    REQUIRE(client->build_time_us == kBuildTimeUs);
  }

  SECTION("Undersized hello request returns error") {
    // Send only the type byte, not the full struct
    beatled_message_t tiny{};
    tiny.type = BEATLED_MESSAGE_HELLO_REQUEST;

    auto buf = make_request(&tiny, sizeof(tiny));
    UDPRequestHandler handler(&buf, sm);

    auto resp = handler.response();
    REQUIRE(resp->type() == BEATLED_MESSAGE_ERROR);
  }
}

TEST_CASE("UDPRequestHandler time request", "[udp][handler]") {
  StateManager sm;

  // Register a client first so the handler can find it
  ClientStatus::board_id_t bid{};
  bid[0] = 'T';
  auto cs = std::make_shared<ClientStatus>(bid, asio::ip::make_address("10.0.0.1"));
  cs->last_status_time = 1;
  sm.register_client(cs);

  SECTION("Valid time request returns three timestamps") {
    beatled_message_time_request_t time_req{};
    time_req.base.type = BEATLED_MESSAGE_TIME_REQUEST;
    uint64_t orig = 5000000ULL;
    time_req.orig_time = htonll(orig);

    auto buf = make_request(&time_req, sizeof(time_req));
    UDPRequestHandler handler(&buf, sm);

    auto resp = handler.response();
    REQUIRE(resp->type() == BEATLED_MESSAGE_TIME_RESPONSE);
    REQUIRE(resp->size() == sizeof(beatled_message_time_response_t));

    const auto *msg = reinterpret_cast<const beatled_message_time_response_t *>(&resp->data());
    REQUIRE(ntohll(msg->orig_time) == orig);
    // recv and xmit should be non-zero timestamps
    REQUIRE(ntohll(msg->recv_time) > 0);
    REQUIRE(ntohll(msg->xmit_time) > 0);
    // xmit should be >= recv (xmit is captured after recv)
    REQUIRE(ntohll(msg->xmit_time) >= ntohll(msg->recv_time));
  }

  SECTION("Undersized time request returns error") {
    beatled_message_t tiny{};
    tiny.type = BEATLED_MESSAGE_TIME_REQUEST;

    auto buf = make_request(&tiny, sizeof(tiny));
    UDPRequestHandler handler(&buf, sm);

    auto resp = handler.response();
    REQUIRE(resp->type() == BEATLED_MESSAGE_ERROR);
  }
}

TEST_CASE("UDPRequestHandler tempo request", "[udp][handler]") {
  StateManager sm;
  sm.update_tempo(120.0f, 99999ULL);
  sm.update_program_id(5);

  SECTION("Tempo request returns current state") {
    beatled_message_tempo_request_t tempo_req{};
    tempo_req.base.type = BEATLED_MESSAGE_TEMPO_REQUEST;

    auto buf = make_request(&tempo_req, sizeof(tempo_req));
    UDPRequestHandler handler(&buf, sm);

    auto resp = handler.response();
    REQUIRE(resp->type() == BEATLED_MESSAGE_TEMPO_RESPONSE);
    REQUIRE(resp->size() == sizeof(beatled_message_tempo_response_t));

    const auto *msg = reinterpret_cast<const beatled_message_tempo_response_t *>(&resp->data());
    REQUIRE(ntohll(msg->beat_time_ref) == 99999ULL);
    REQUIRE(ntohl(msg->tempo_period_us) == 500000); // 60M / 120
    REQUIRE(ntohs(msg->program_id) == 5);
  }

  SECTION("v4 status response decodes qos + rtt onto ClientStatus") {
    ClientStatus::board_id_t bid{};
    bid[0] = 'R';
    auto cs = std::make_shared<ClientStatus>(bid, asio::ip::make_address("10.0.0.1"));
    cs->last_status_time = 1;
    sm.register_client(cs);

    beatled_message_status_response_t resp{};
    resp.base.type = BEATLED_MESSAGE_STATUS_RESPONSE;
    // Pick a send time slightly in the past so RTT > 0 after the
    // handler stamps now.
    const uint64_t kSendTime = 1;
    resp.echo_server_send_time_us = htonll(kSendTime);
    resp.qos.median_rtt_us = htonl(7777);
    resp.qos.next_beat_gap_total = htonl(9);
    resp.qos.intercore_drop_total = htonl(3);
    resp.qos.time_sync_outlier_total = htonl(13);
    resp.qos.valid_sample_count = htons(6);
    resp.qos.last_applied_program_seq = htons(99);

    auto buf = make_request(&resp, sizeof(resp));
    UDPRequestHandler handler(&buf, sm);
    auto reply = handler.response();
    REQUIRE(reply == nullptr); // STATUS_RESPONSE is terminal — no reply

    auto stored = sm.client_status(asio::ip::make_address("10.0.0.1"));
    REQUIRE(stored != nullptr);
    REQUIRE(stored->latest_qos.valid);
    REQUIRE(stored->latest_qos.median_rtt_us == 7777u);
    REQUIRE(stored->latest_qos.next_beat_gap_total == 9u);
    REQUIRE(stored->latest_qos.intercore_drop_total == 3u);
    REQUIRE(stored->latest_qos.time_sync_outlier_total == 13u);
    REQUIRE(stored->latest_qos.valid_sample_count == 6u);
    REQUIRE(stored->latest_qos.last_applied_program_seq == 99u);
    REQUIRE(stored->latest_qos.last_rtt_us > 0u);
  }

  SECTION("v4 qos block decodes onto ClientStatus") {
    // Register a client at the request's source IP so the handler can
    // attach the QoS snapshot to it.
    ClientStatus::board_id_t bid{};
    bid[0] = 'Q';
    auto cs = std::make_shared<ClientStatus>(bid, asio::ip::make_address("10.0.0.1"));
    cs->last_status_time = 1;
    sm.register_client(cs);

    beatled_message_tempo_request_t tempo_req{};
    tempo_req.base.type = BEATLED_MESSAGE_TEMPO_REQUEST;
    tempo_req.owd_us_estimate = htonl(2500);

    const int64_t kOffsetUs = -12345;
    const uint64_t kUptimeUs = 12'345'678ULL;
    uint64_t off_bits;
    std::memcpy(&off_bits, &kOffsetUs, sizeof(off_bits));
    tempo_req.qos.current_offset_us = static_cast<int64_t>(htonll(off_bits));
    tempo_req.qos.uptime_us = htonll(kUptimeUs);
    tempo_req.qos.median_rtt_us = htonl(4321);
    tempo_req.qos.next_beat_gap_total = htonl(7);
    tempo_req.qos.intercore_drop_total = htonl(2);
    tempo_req.qos.time_sync_outlier_total = htonl(11);
    tempo_req.qos.valid_sample_count = htons(8);
    tempo_req.qos.last_applied_program_seq = htons(42);

    auto buf = make_request(&tempo_req, sizeof(tempo_req));
    UDPRequestHandler handler(&buf, sm);
    auto resp = handler.response();
    REQUIRE(resp->type() == BEATLED_MESSAGE_TEMPO_RESPONSE);

    auto stored = sm.client_status(asio::ip::make_address("10.0.0.1"));
    REQUIRE(stored != nullptr);
    const auto &qos = stored->latest_qos;
    REQUIRE(qos.valid);
    REQUIRE(qos.current_offset_us == kOffsetUs);
    REQUIRE(qos.uptime_us == kUptimeUs);
    REQUIRE(qos.median_rtt_us == 4321u);
    REQUIRE(qos.next_beat_gap_total == 7u);
    REQUIRE(qos.intercore_drop_total == 2u);
    REQUIRE(qos.time_sync_outlier_total == 11u);
    REQUIRE(qos.valid_sample_count == 8u);
    REQUIRE(qos.last_applied_program_seq == 42u);
    REQUIRE(qos.server_received_at_us > 0u);
  }
}
