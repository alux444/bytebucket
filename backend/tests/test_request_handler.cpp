#include <catch2/catch_test_macros.hpp>
#include "request_handler.hpp"
#include <boost/beast/http.hpp>
#include <string>

namespace http = boost::beast::http;

// Helper function to create HTTP requests for testing
http::request<http::string_body> make_request(http::verb method, std::string_view target, std::string body = "")
{
  http::request<http::string_body> req{method, target, 11}; // HTTP/1.1
  req.set(http::field::host, "localhost");
  req.set(http::field::user_agent, "test-client");
  if (!body.empty())
  {
    req.body() = body;
    req.prepare_payload();
  }
  return req;
}

TEST_CASE("Health endpoint returns OK status", "[health]")
{
  auto req = make_request(http::verb::get, "/health");

  // Test that the function executes without throwing
  REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
}

TEST_CASE("Root endpoint returns ByteBucket message", "[root]")
{
  auto req = make_request(http::verb::get, "/");

  REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
}

TEST_CASE("Upload endpoint accepts POST requests", "[upload]")
{
  auto req = make_request(http::verb::post, "/upload", "test file content");

  REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
}

TEST_CASE("Download endpoint with valid ID", "[download]")
{
  auto req = make_request(http::verb::get, "/download/test123");

  REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
}

TEST_CASE("Download endpoint with invalid ID", "[download]")
{
  auto req = make_request(http::verb::get, "/download/invalid");

  REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
}

TEST_CASE("Download endpoint with empty ID", "[download]")
{
  auto req = make_request(http::verb::get, "/download/");

  REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
}

TEST_CASE("Unknown endpoint returns 404", "[404]")
{
  auto req = make_request(http::verb::get, "/nonexistent");

  REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
}

TEST_CASE("HTTP method validation", "[methods]")
{
  SECTION("GET to upload should return 404")
  {
    auto req = make_request(http::verb::get, "/upload");
    REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
  }

  SECTION("POST to health should return 404")
  {
    auto req = make_request(http::verb::post, "/health");
    REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
  }

  SECTION("DELETE method should return 404")
  {
    auto req = make_request(http::verb::delete_, "/health");
    REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
  }
}

TEST_CASE("Request body handling", "[body]")
{
  SECTION("Upload with empty body")
  {
    auto req = make_request(http::verb::post, "/upload", "");
    REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
  }

  SECTION("Upload with large body")
  {
    std::string large_body(1024, 'A'); // 1KB of 'A' characters
    auto req = make_request(http::verb::post, "/upload", large_body);
    REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
  }

  SECTION("Upload with special characters")
  {
    std::string special_body = "Special chars: àáâãäåæçèéêë 你好 🚀";
    auto req = make_request(http::verb::post, "/upload", special_body);
    REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
  }
}

TEST_CASE("URL path parsing", "[paths]")
{
  SECTION("Download with numeric ID")
  {
    auto req = make_request(http::verb::get, "/download/12345");
    REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
  }

  SECTION("Download with alphanumeric ID")
  {
    auto req = make_request(http::verb::get, "/download/abc123def");
    REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
  }

  SECTION("Download with special characters in ID")
  {
    auto req = make_request(http::verb::get, "/download/file-name_with.ext");
    REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
  }

  SECTION("Download with very long ID")
  {
    std::string long_id(256, 'x'); // 256 character ID
    auto req = make_request(http::verb::get, "/download/" + long_id);
    REQUIRE_NOTHROW(bytebucket::handle_request(std::move(req)));
  }
}
