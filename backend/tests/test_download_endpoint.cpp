#include <catch2/catch_test_macros.hpp>
#include <boost/beast/http.hpp>
#include "request_handler.hpp"
#include "test_helpers.hpp"

TEST_CASE("Download endpoint tests", "[download]")
{
  using namespace boost::beast::http;

  SECTION("GET /download/test123 returns test file")
  {
    request<string_body> req{verb::get, "/download/test123", 11};
    req.set(field::host, "localhost");
    req.set(field::user_agent, "test-client");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::ok);
    REQUIRE(response[field::server] == "ByteBucket-Server");
    REQUIRE(response[field::content_type] == "text/plain");
    REQUIRE(response[field::content_disposition] == "attachment; filename=\"test_file_test123.txt\"");
    REQUIRE(response.body() == "Found file ID! test123");
  }

  SECTION("GET /download/nonexistent returns 404")
  {
    request<string_body> req{verb::get, "/download/nonexistent", 11};
    req.set(field::host, "localhost");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::not_found);
    REQUIRE(response[field::server] == "ByteBucket-Server");
    REQUIRE(response[field::content_type] == "application/json");
    REQUIRE(response.body() == R"({"error":"File not found"})");
  }

  SECTION("GET /download/ (empty ID) returns 400")
  {
    request<string_body> req{verb::get, "/download/", 11};
    req.set(field::host, "localhost");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::bad_request);
    REQUIRE(response[field::server] == "ByteBucket-Server");
    REQUIRE(response[field::content_type] == "application/json");
    REQUIRE(response.body() == R"({"error":"File ID is required"})");
  }

  SECTION("POST /download/test123 should return 404")
  {
    request<string_body> req{verb::post, "/download/test123", 11};
    req.set(field::host, "localhost");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::not_found);
    REQUIRE(response.body() == "Not found");
  }
}
