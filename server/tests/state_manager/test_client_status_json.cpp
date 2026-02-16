#include <catch2/catch_test_macros.hpp>
#include <core/client_status.hpp>
#include <nlohmann/json.hpp>
#include <asio.hpp>

using beatled::core::ClientStatus;
using json = nlohmann::json;

TEST_CASE("ClientStatus JSON serialization", "[client_status][json]") {

  SECTION("board_id should serialize as hex string, not integer array") {
    // Create a board_id with known values (simulating actual device ID)
    ClientStatus::board_id_t board_id;
    // Fill with test data: -66,-83,80,88,0,0,54,-25 from user's output
    board_id[0] = static_cast<char>(-66);  // 0xBE
    board_id[1] = static_cast<char>(-83);  // 0xAD
    board_id[2] = 'P';   // 80
    board_id[3] = 'X';   // 88
    board_id[4] = 0;
    board_id[5] = 0;
    board_id[6] = static_cast<char>(54);   // 0x36
    board_id[7] = static_cast<char>(-25);  // 0xE7
    for (int i = 8; i < 17; i++) {
      board_id[i] = 0;
    }

    // Create ClientStatus
    asio::ip::address ip_addr = asio::ip::make_address("127.0.0.1");
    auto cs = std::make_shared<ClientStatus>(board_id, ip_addr);
    cs->client_id = 0;
    cs->last_status_time = 1771268537669552;

    // Serialize to JSON using the NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE macro
    json j = *cs;

    // Print the JSON to see what we actually get
    INFO("Serialized JSON: " << j.dump(2));

    // Check that board_id exists
    REQUIRE(j.contains("board_id"));

    // CRITICAL: board_id should be a STRING, not an ARRAY
    REQUIRE(j["board_id"].is_string());

    // If it's a string, verify it's non-empty
    std::string board_id_str = j["board_id"];
    REQUIRE(!board_id_str.empty());

    // Verify it doesn't contain the raw bytes (which would fail above anyway)
    REQUIRE_FALSE(j["board_id"].is_array());
  }

  SECTION("board_id_t direct serialization should produce string") {
    ClientStatus::board_id_t board_id;
    board_id[0] = 'T';
    board_id[1] = 'E';
    board_id[2] = 'S';
    board_id[3] = 'T';
    board_id[4] = '\0';
    for (int i = 5; i < 17; i++) {
      board_id[i] = 0;
    }

    // Directly serialize board_id_t
    json j = board_id;

    INFO("Direct board_id serialization: " << j.dump());

    // Should be a string
    REQUIRE(j.is_string());

    // Should be "TEST" (stops at first null terminator)
    std::string result = j;
    REQUIRE(result == "TEST");
  }
}
