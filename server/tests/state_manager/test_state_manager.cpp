#include <catch2/catch_test_macros.hpp>
#include <core/state_manager.hpp>
#include <thread>

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

  SECTION("Tempo can be updated multiple times") {
    sm.update_tempo(120.0f, 1000);
    sm.update_tempo(140.0f, 2000);
    tempo_ref_t tr = sm.get_tempo_ref();
    REQUIRE(tr.tempo == 140.0f);
    REQUIRE(tr.beat_time_ref == 2000);
  }

  SECTION("Tempo period calculation is correct for common BPMs") {
    sm.update_tempo(120.0f, 0);
    REQUIRE(sm.get_tempo_ref().tempo_period_us == 500000); // 60M / 120

    sm.update_tempo(60.0f, 0);
    REQUIRE(sm.get_tempo_ref().tempo_period_us == 1000000); // 60M / 60

    sm.update_tempo(180.0f, 0);
    REQUIRE(sm.get_tempo_ref().tempo_period_us == 333333); // 60M / 180
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

  SECTION("Re-registering same board_id replaces old entry") {
    asio::ip::address addr1 = asio::ip::make_address("10.0.0.1");
    asio::ip::address addr2 = asio::ip::make_address("10.0.0.2");

    ClientStatus::board_id_t board_id{};
    board_id[0] = 'A';

    auto cs1 = std::make_shared<ClientStatus>(board_id, addr1);
    sm.register_client(cs1);
    REQUIRE(sm.client_status(addr1) == cs1);

    auto cs2 = std::make_shared<ClientStatus>(board_id, addr2);
    sm.register_client(cs2);
    REQUIRE_FALSE(sm.client_status(addr1));
    REQUIRE(sm.client_status(addr2) == cs2);
  }

  SECTION("Re-registering same IP with different board replaces old entry") {
    asio::ip::address addr = asio::ip::make_address("10.0.0.1");

    ClientStatus::board_id_t board_id1{};
    board_id1[0] = 'A';
    ClientStatus::board_id_t board_id2{};
    board_id2[0] = 'B';

    auto cs1 = std::make_shared<ClientStatus>(board_id1, addr);
    sm.register_client(cs1);
    REQUIRE(sm.client_status(board_id1));

    auto cs2 = std::make_shared<ClientStatus>(board_id2, addr);
    sm.register_client(cs2);
    REQUIRE_FALSE(sm.client_status(board_id1));
    REQUIRE(sm.client_status(board_id2) == cs2);
  }
}

TEST_CASE("StateManager program", "[state_manager]") {
  StateManager sm;

  SECTION("Program ID defaults to 0") {
    REQUIRE(sm.get_program_id() == 0);
  }

  SECTION("Program ID can be set and retrieved") {
    sm.update_program_id(5);
    REQUIRE(sm.get_program_id() == 5);

    sm.update_program_id(0);
    REQUIRE(sm.get_program_id() == 0);
  }
}

TEST_CASE("StateManager next beat", "[state_manager]") {
  StateManager sm;

  SECTION("Next beat time can be set and triggers callbacks") {
    uint64_t received_time = 0;
    sm.register_next_beat_cb([&](uint64_t t) { received_time = t; });

    sm.update_next_beat(999999);
    REQUIRE(sm.get_next_beat_time_ref() == 999999);
    REQUIRE(received_time == 999999);
  }

  SECTION("Multiple callbacks are all invoked") {
    int call_count = 0;
    sm.register_next_beat_cb([&](uint64_t) { call_count++; });
    sm.register_next_beat_cb([&](uint64_t) { call_count++; });

    sm.update_next_beat(1000);
    REQUIRE(call_count == 2);
  }
}

TEST_CASE("StateManager thread safety", "[state_manager]") {
  StateManager sm;

  SECTION("Concurrent tempo updates do not crash") {
    auto writer = [&]() {
      for (int i = 0; i < 1000; i++) {
        sm.update_tempo(static_cast<float>(60 + i % 120), i * 1000);
      }
    };

    auto reader = [&]() {
      for (int i = 0; i < 1000; i++) {
        tempo_ref_t tr = sm.get_tempo_ref();
        (void)tr;
      }
    };

    std::thread t1(writer);
    std::thread t2(reader);
    std::thread t3(writer);

    t1.join();
    t2.join();
    t3.join();

    // If we get here without crashing, the test passes
    tempo_ref_t final_tr = sm.get_tempo_ref();
    REQUIRE(final_tr.tempo > 0);
  }

  SECTION("Concurrent client registration does not crash") {
    auto registerer = [&](int offset) {
      for (int i = 0; i < 100; i++) {
        ClientStatus::board_id_t bid{};
        bid[0] = static_cast<char>(offset + i);
        auto addr = asio::ip::make_address(
            "10.0." + std::to_string(offset) + "." + std::to_string(i % 256));
        auto cs = std::make_shared<ClientStatus>(bid, addr);
        sm.register_client(cs);
      }
    };

    std::thread t1(registerer, 0);
    std::thread t2(registerer, 100);

    t1.join();
    t2.join();
  }
}
