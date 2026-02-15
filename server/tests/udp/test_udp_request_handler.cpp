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
UDPRequestBuffer make_request(const void *msg, size_t size,
                              const char *ip = "10.0.0.1") {
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

    const auto *err =
        reinterpret_cast<const beatled_message_error_t *>(&resp->data());
    REQUIRE(err->error_code == BEATLED_ERROR_NO_DATA);
  }

  SECTION("Unknown message type returns UNKNOWN_MESSAGE_TYPE error") {
    uint8_t bad_type = 0xFF;
    auto buf = make_request(&bad_type, 1);
    UDPRequestHandler handler(&buf, sm);

    auto resp = handler.response();
    REQUIRE(resp->type() == BEATLED_MESSAGE_ERROR);

    const auto *err =
        reinterpret_cast<const beatled_message_error_t *>(&resp->data());
    REQUIRE(err->error_code == BEATLED_ERROR_UNKNOWN_MESSAGE_TYPE);
  }
}

TEST_CASE("UDPRequestHandler hello request", "[udp][handler]") {
  StateManager sm;

  SECTION("Valid hello request registers client and returns response") {
    beatled_message_hello_request_t hello_req{};
    hello_req.base.type = BEATLED_MESSAGE_HELLO_REQUEST;
    std::memcpy(hello_req.board_id, "ABCD1234ABCD1234", sizeof(hello_req.board_id));

    auto buf = make_request(&hello_req, sizeof(hello_req));
    UDPRequestHandler handler(&buf, sm);

    auto resp = handler.response();
    REQUIRE(resp->type() == BEATLED_MESSAGE_HELLO_RESPONSE);
    REQUIRE(resp->size() == sizeof(beatled_message_hello_response_t));

    // Client should now be registered
    auto client = sm.client_status(asio::ip::make_address("10.0.0.1"));
    REQUIRE(client != nullptr);
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

    const auto *msg =
        reinterpret_cast<const beatled_message_time_response_t *>(&resp->data());
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

    const auto *msg =
        reinterpret_cast<const beatled_message_tempo_response_t *>(&resp->data());
    REQUIRE(ntohll(msg->beat_time_ref) == 99999ULL);
    REQUIRE(ntohl(msg->tempo_period_us) == 500000); // 60M / 120
    REQUIRE(ntohs(msg->program_id) == 5);
  }
}
