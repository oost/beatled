// Smoke test for make_tls_context: certificate loading is the part of the
// HTTPS bring-up that fails in the field (missing/garbled PEMs), so it is
// split out of the HTTPServer constructor and exercised here against
// checked-in, test-only self-signed fixtures (fixtures/tls/).

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <fstream>

#include "http_server/http_server.hpp"

using beatled::server::make_tls_context;

namespace {

std::filesystem::path fixtures_dir() {
  return std::filesystem::path(TEST_FIXTURES_DIR) / "tls";
}

} // namespace

TEST_CASE("valid cert, key and dh params load", "[tls]") {
  const auto dir = fixtures_dir();
  REQUIRE_NOTHROW(make_tls_context(dir / "cert.pem", dir / "key.pem", dir / "dh_param.pem"));
}

TEST_CASE("a missing file is reported by name", "[tls]") {
  const auto dir = fixtures_dir();
  REQUIRE_THROWS_WITH(
      make_tls_context(dir / "nonexistent.pem", dir / "key.pem", dir / "dh_param.pem"),
      Catch::Matchers::ContainsSubstring("nonexistent.pem"));
}

TEST_CASE("garbled PEM contents are rejected", "[tls]") {
  const auto dir = std::filesystem::temp_directory_path() / "beatled_tls_test";
  std::filesystem::create_directories(dir);
  for (const char *name : {"cert.pem", "key.pem", "dh_param.pem"}) {
    std::ofstream(dir / name) << "not a pem file\n";
  }

  REQUIRE_THROWS(make_tls_context(dir / "cert.pem", dir / "key.pem", dir / "dh_param.pem"));

  std::filesystem::remove_all(dir);
}
