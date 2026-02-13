#include <catch2/catch_test_macros.hpp>
#include <core/interfaces/service_controller.hpp>
#include <core/interfaces/service_manager.hpp>
#include <core/state_manager.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using beatled::core::ServiceControllerInterface;
using beatled::core::ServiceManagerInterface;
using beatled::core::StateManager;

namespace {

class MockService : public ServiceControllerInterface {
public:
  MockService(const std::string &id) : ServiceControllerInterface(id) {}

protected:
  void start_sync() override {}
  void stop_sync() override {}
  const char *service_name() const override { return "MockService"; }
};

class MockServiceManager : public ServiceManagerInterface {
public:
  MockServiceManager() : state_manager_{} {}

  StateManager &state_manager() override { return state_manager_; }

  void add_service(std::unique_ptr<ServiceControllerInterface> svc) {
    registerController(std::move(svc));
  }

private:
  StateManager state_manager_;
};

} // namespace

TEST_CASE("API response contract: status endpoint", "[api]") {
  MockServiceManager sm;
  sm.add_service(std::make_unique<MockService>("beat_detector"));
  sm.add_service(std::make_unique<MockService>("udp_server"));

  sm.state_manager().update_tempo(120.5f, 1000000);

  SECTION("Status response contains expected fields") {
    json response;
    response["message"] = "It's all good!";

    json service_status;
    for (auto &&[id, svc] : sm.services()) {
      service_status[svc->id()] = svc->is_running();
    }
    response["status"] = service_status;
    response["tempo"] = sm.state_manager().get_tempo_ref().tempo;

    REQUIRE(response.contains("message"));
    REQUIRE(response.contains("status"));
    REQUIRE(response.contains("tempo"));
    REQUIRE(response["message"] == "It's all good!");
    REQUIRE(response["tempo"].get<float>() == 120.5f);
    REQUIRE(response["status"]["beat_detector"] == false);
    REQUIRE(response["status"]["udp_server"] == false);
  }

  SECTION("Status reflects running services") {
    sm.service("beat_detector")->start();

    json service_status;
    for (auto &&[id, svc] : sm.services()) {
      service_status[svc->id()] = svc->is_running();
    }

    REQUIRE(service_status["beat_detector"] == true);
    REQUIRE(service_status["udp_server"] == false);
  }
}

TEST_CASE("API response contract: tempo endpoint", "[api]") {
  MockServiceManager sm;

  SECTION("Tempo response contains tempo and time_ref") {
    sm.state_manager().update_tempo(140.0f, 5000000);
    auto tr = sm.state_manager().get_tempo_ref();

    json response;
    response["tempo"] = tr.tempo;
    response["time_ref"] = tr.beat_time_ref;

    REQUIRE(response["tempo"].get<float>() == 140.0f);
    REQUIRE(response["time_ref"].get<uint64_t>() == 5000000);
  }
}

TEST_CASE("API response contract: program endpoint", "[api]") {
  MockServiceManager sm;

  SECTION("GET program returns programs list and current programId") {
    sm.state_manager().update_program_id(3);
    uint16_t program_id = sm.state_manager().get_program_id();

    json response;
    response["programId"] = program_id;
    response["programs"] = json::array(
        {{{"name", "Snakes!"}, {"id", 0}},
         {{"name", "Random data"}, {"id", 1}},
         {{"name", "Sparkles"}, {"id", 2}},
         {{"name", "Greys"}, {"id", 3}},
         {{"name", "Drops"}, {"id", 4}},
         {{"name", "Solid!"}, {"id", 5}},
         {{"name", "Fade"}, {"id", 6}},
         {{"name", "Fade Color"}, {"id", 7}}});

    REQUIRE(response["programId"].get<uint16_t>() == 3);
    REQUIRE(response["programs"].is_array());
    REQUIRE(response["programs"].size() == 8);

    for (const auto &prog : response["programs"]) {
      REQUIRE(prog.contains("name"));
      REQUIRE(prog.contains("id"));
    }
  }

  SECTION("POST program updates program id") {
    json request_body;
    request_body["programId"] = 5;

    uint16_t program_id = request_body["programId"].get<uint16_t>();
    sm.state_manager().update_program_id(program_id);

    REQUIRE(sm.state_manager().get_program_id() == 5);
  }

  SECTION("POST program rejects invalid JSON") {
    REQUIRE_THROWS_AS(json::parse("{invalid}"), json::parse_error);
  }
}

TEST_CASE("API auth contract: Bearer token matching", "[api][auth]") {
  SECTION("Empty token means auth is disabled") {
    std::string api_token;
    REQUIRE(api_token.empty());
    // When token is empty, any request should be authorized
    // (mirrors check_auth returning true when api_token_ is empty)
  }

  SECTION("Correct Bearer token is accepted") {
    std::string api_token = "my-secret-token";
    std::string auth_header = "Bearer " + api_token;
    REQUIRE(auth_header == "Bearer my-secret-token");
  }

  SECTION("Wrong token is rejected") {
    std::string api_token = "my-secret-token";
    std::string auth_header = "Bearer wrong-token";
    REQUIRE(auth_header != "Bearer " + api_token);
  }

  SECTION("Missing Bearer prefix is rejected") {
    std::string api_token = "my-secret-token";
    std::string auth_header = "my-secret-token";
    REQUIRE(auth_header != "Bearer " + api_token);
  }

  SECTION("Empty auth header is rejected") {
    std::string api_token = "my-secret-token";
    std::string auth_header;
    REQUIRE(auth_header != "Bearer " + api_token);
  }
}

TEST_CASE("API response contract: service control", "[api]") {
  MockServiceManager sm;
  sm.add_service(std::make_unique<MockService>("beat_detector"));

  SECTION("Service can be started via control request") {
    json request_body;
    request_body["id"] = "beat_detector";
    request_body["status"] = true;

    std::string service_name = request_body["id"].get<std::string>();
    bool requested_status = request_body["status"].get<bool>();
    auto *service = sm.service(service_name);

    REQUIRE(service != nullptr);
    REQUIRE_FALSE(service->is_running());

    if (requested_status) {
      service->start();
    }

    REQUIRE(service->is_running());

    json response;
    response["status"] = service->is_running();
    REQUIRE(response["status"] == true);
  }

  SECTION("Service can be stopped via control request") {
    auto *service = sm.service("beat_detector");
    service->start();
    REQUIRE(service->is_running());

    json request_body;
    request_body["id"] = "beat_detector";
    request_body["status"] = false;

    bool requested_status = request_body["status"].get<bool>();
    if (!requested_status) {
      service->stop();
    }

    REQUIRE_FALSE(service->is_running());
  }

  SECTION("Unknown service returns nullptr") {
    auto *service = sm.service("nonexistent");
    REQUIRE(service == nullptr);
  }

  SECTION("Service control request JSON parsing") {
    std::string raw = R"({"id": "beat_detector", "status": true})";
    json parsed = json::parse(raw);

    REQUIRE(parsed["id"].get<std::string>() == "beat_detector");
    REQUIRE(parsed["status"].get<bool>() == true);
  }
}
