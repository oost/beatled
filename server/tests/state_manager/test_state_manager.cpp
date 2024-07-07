#include <catch2/catch_test_macros.hpp>
#include <core/state_manager.hpp>

using beatled::core::ClientStatus;
using beatled::core::StateManager;
using beatled::core::tempo_ref_t;

TEST_CASE("StateManager tempo", "[state_manager]") {
  StateManager sm;

  SECTION("Tempo can be set") {
    float tempo = 1.1;
    uint64_t timeref = 12345;
    sm.update_tempo(tempo, timeref);
    tempo_ref_t time_ref = sm.get_tempo_ref();
    REQUIRE(time_ref.tempo == tempo);
    REQUIRE(time_ref.tempo_period_us ==
            (static_cast<uint32_t>(1000000 * 60 / tempo)));
    REQUIRE(time_ref.beat_time_ref == timeref);
  }

  SECTION("Clients can be registered") {
    asio::ip::address client_address = asio::ip::make_address("127.0.0.1");

    asio::ip::address client_address2 = asio::ip::make_address("127.0.0.2");

    ClientStatus::board_id_t board_id;
    board_id[0] = 1;

    REQUIRE_FALSE(sm.client_status(client_address));
    REQUIRE_FALSE(sm.client_status(client_address2));

    ClientStatus::Ptr cs =
        std::make_shared<ClientStatus>(board_id, client_address);
    sm.register_client(cs);

    REQUIRE(sm.client_status(client_address) == cs);
    REQUIRE_FALSE(sm.client_status(client_address2));
  }
}