#include <catch2/catch_test_macros.hpp>
#include <string_view>

#include "../../src/server/http/response_handler.hpp"

using namespace beatled::server;

// ResponseHandler::content_type_by_file_extention is protected and static.
// Expose it via a test subclass.
class TestableResponseHandler : public ResponseHandler {
public:
  static const char *mime(const std::string_view ext) {
    return content_type_by_file_extention(ext);
  }
};

TEST_CASE("MIME type mapping for common web types", "[http][mime]") {
  REQUIRE(std::string_view(TestableResponseHandler::mime("html")) == "text/html");
  REQUIRE(std::string_view(TestableResponseHandler::mime("htm")) == "text/html");
  REQUIRE(std::string_view(TestableResponseHandler::mime("css")) == "text/css");
  REQUIRE(std::string_view(TestableResponseHandler::mime("js")) == "application/javascript");
  REQUIRE(std::string_view(TestableResponseHandler::mime("json")) == "application/json");
  REQUIRE(std::string_view(TestableResponseHandler::mime("png")) == "image/png");
  REQUIRE(std::string_view(TestableResponseHandler::mime("jpg")) == "image/jpeg");
  REQUIRE(std::string_view(TestableResponseHandler::mime("jpeg")) == "image/jpeg");
  REQUIRE(std::string_view(TestableResponseHandler::mime("gif")) == "image/gif");
  REQUIRE(std::string_view(TestableResponseHandler::mime("svg")) == "image/svg+xml");
  REQUIRE(std::string_view(TestableResponseHandler::mime("ico")) == "image/x-icon");
  REQUIRE(std::string_view(TestableResponseHandler::mime("pdf")) == "application/pdf");
  REQUIRE(std::string_view(TestableResponseHandler::mime("xml")) == "application/xml");
  REQUIRE(std::string_view(TestableResponseHandler::mime("csv")) == "text/csv");
}

TEST_CASE("MIME type mapping for font types", "[http][mime]") {
  REQUIRE(std::string_view(TestableResponseHandler::mime("woff")) == "font/woff");
  REQUIRE(std::string_view(TestableResponseHandler::mime("woff2")) == "font/woff2");
  REQUIRE(std::string_view(TestableResponseHandler::mime("ttf")) == "font/ttf");
  REQUIRE(std::string_view(TestableResponseHandler::mime("otf")) == "font/otf");
  REQUIRE(std::string_view(TestableResponseHandler::mime("eot")) == "application/vnd.ms-fontobject");
}

TEST_CASE("MIME type mapping for media types", "[http][mime]") {
  REQUIRE(std::string_view(TestableResponseHandler::mime("mp3")) == "application/text"); // not mapped
  REQUIRE(std::string_view(TestableResponseHandler::mime("wav")) == "audio/x-wav");
  REQUIRE(std::string_view(TestableResponseHandler::mime("mpeg")) == "video/mpeg");
  REQUIRE(std::string_view(TestableResponseHandler::mime("webm")) == "video/webm");
  REQUIRE(std::string_view(TestableResponseHandler::mime("webp")) == "image/webp");
}

TEST_CASE("MIME type mapping for archive types", "[http][mime]") {
  REQUIRE(std::string_view(TestableResponseHandler::mime("rar")) == "application/x-rar-compressed");
  REQUIRE(std::string_view(TestableResponseHandler::mime("zip")) == "application/zip");
  REQUIRE(std::string_view(TestableResponseHandler::mime("7z")) == "application/x-7z-compressed");
  REQUIRE(std::string_view(TestableResponseHandler::mime("tar")) == "application/x-tar");
}

TEST_CASE("MIME type for webmanifest", "[http][mime]") {
  REQUIRE(std::string_view(TestableResponseHandler::mime("webmanifest")) ==
          "application/manifest+json");
}

TEST_CASE("Unknown extension returns default MIME type", "[http][mime]") {
  REQUIRE(std::string_view(TestableResponseHandler::mime("xyz")) == "application/text");
  REQUIRE(std::string_view(TestableResponseHandler::mime("unknown")) == "application/text");
  REQUIRE(std::string_view(TestableResponseHandler::mime("")) == "application/text");
}
