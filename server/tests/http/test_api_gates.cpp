// E2E tests for the two cross-cutting HTTP gates that every authenticated
// API endpoint passes through: bearer-token authorization and the sliding
// 60-requests-per-10s rate limiter.
//
// We can't easily fabricate a real restinio::request_handle_t in a unit
// test, so the auth side reaches for the small static helper
// APIHandler::authorize_request that the production check_auth() delegates
// to. The rate-limit side does not need a request object at all, so we
// hit APIHandler::check_rate_limit() directly.

#include <catch2/catch_test_macros.hpp>

#include "core/state_manager.hpp"
#include "logger/logger.hpp"

// API handler header lives next to its translation unit in src/server/http/,
// not under a public include directory. test_mime_types.cpp uses the same
// relative include style.
#include "../../src/server/http/api_handler.hpp"

#include <chrono>
#include <thread>

using beatled::core::ServiceControllerInterface;
using beatled::core::ServiceManagerInterface;
using beatled::core::StateManager;
using beatled::server::APIHandler;
using beatled::server::Logger;

namespace {

// Minimal service manager — APIHandler holds a reference but the gate
// tests never reach the service map.
class TestServiceManager : public ServiceManagerInterface {
public:
  StateManager &state_manager() override { return state_manager_; }

private:
  StateManager state_manager_;
};

APIHandler make_handler(std::string token) {
  static TestServiceManager sm;
  static Logger logger{Logger::parameters_t{}};
  return APIHandler(sm, logger, /*cors_origin=*/"", std::move(token));
}

} // namespace

TEST_CASE("authorize_request: token disabled = allow everyone", "[api][auth]") {
  REQUIRE(APIHandler::authorize_request(std::nullopt, /*api_token=*/""));
  REQUIRE(APIHandler::authorize_request(std::string{"Bearer anything"}, ""));
}

TEST_CASE("authorize_request: token set, missing header = 401", "[api][auth]") {
  REQUIRE_FALSE(APIHandler::authorize_request(std::nullopt, "s3cret"));
}

TEST_CASE("authorize_request: token set, wrong value = 401", "[api][auth]") {
  REQUIRE_FALSE(APIHandler::authorize_request(std::string{"Bearer wrong"}, "s3cret"));
  REQUIRE_FALSE(APIHandler::authorize_request(std::string{"Bearer s3cretX"}, "s3cret"));
  REQUIRE_FALSE(APIHandler::authorize_request(std::string{"Xs3cret"}, "s3cret"));
  REQUIRE_FALSE(APIHandler::authorize_request(std::string{"s3cret"}, "s3cret"));
}

TEST_CASE("authorize_request: token set, correct Bearer value = 200", "[api][auth]") {
  REQUIRE(APIHandler::authorize_request(std::string{"Bearer s3cret"}, "s3cret"));
}

TEST_CASE("check_rate_limit: allows up to kMaxRequestsPerWindow per window", "[api][rate_limit]") {
  auto handler = make_handler("s3cret");

  // Drive the limiter up to the documented threshold (60 per 10 s).
  // The check is a sliding window, so all 60 should pass when called
  // back-to-back.
  for (int i = 0; i < 60; i++) {
    REQUIRE(handler.check_rate_limit());
  }
  // 61st within the window must be rejected.
  REQUIRE_FALSE(handler.check_rate_limit());
}

TEST_CASE("check_rate_limit: window slides forward as old samples expire",
          "[api][rate_limit][.slow]") {
  // Skipped by default ([.slow] tag) because waiting kWindowDuration would
  // make the suite take 10+ seconds on every run. Enable manually with
  // `test_api_gates "[.slow]"` to verify the slide is real.
  auto handler = make_handler("s3cret");
  for (int i = 0; i < 60; i++) {
    REQUIRE(handler.check_rate_limit());
  }
  REQUIRE_FALSE(handler.check_rate_limit());

  std::this_thread::sleep_for(std::chrono::seconds(11));
  REQUIRE(handler.check_rate_limit());
}
