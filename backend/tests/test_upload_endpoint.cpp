#include <catch2/catch_test_macros.hpp>
#include <boost/beast/http.hpp>
#include "request_handler.hpp"
#include "test_helpers.hpp"

TEST_CASE("Upload endpoint tests", "[upload]")
{
  using namespace boost::beast::http;

  SECTION("POST /upload echoes request body")
  {
    request<string_body> req{verb::post, "/upload", 11};
    req.set(field::host, "localhost");
    req.set(field::user_agent, "test-client");
    req.set(field::content_type, "text/plain");
    req.body() = "This is test file content";
    req.prepare_payload();

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::ok);
    REQUIRE(response[field::server] == "ByteBucket-Server");
    REQUIRE(response[field::content_type] == "application/octet-stream");
    REQUIRE(response[field::content_disposition] == "attachment; filename=\"uploaded_file\"");
    REQUIRE(response.body() == "This is test file content");
  }

  SECTION("POST /upload with empty body")
  {
    request<string_body> req{verb::post, "/upload", 11};
    req.set(field::host, "localhost");
    req.body() = "";
    req.prepare_payload();

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::ok);
    REQUIRE(response.body() == "");
  }

  SECTION("GET /upload should return 404")
  {
    request<string_body> req{verb::get, "/upload", 11};
    req.set(field::host, "localhost");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::not_found);
    REQUIRE(response.body() == "Not found");
  }

  SECTION("POST /upload with large content")
  {
    request<string_body> req{verb::post, "/upload", 11};
    req.set(field::host, "localhost");

    std::string large_content(1024, 'A'); // 1KB of 'A' characters
    req.body() = large_content;
    req.prepare_payload();

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::ok);
    REQUIRE(response.body() == large_content);
    REQUIRE(response.body().size() == 1024);
  }
}
