#include <catch2/catch_test_macros.hpp>
#include <boost/beast/http.hpp>
#include "request_handler.hpp"
#include "test_helpers.hpp"

TEST_CASE("Unknown endpoint tests", "[unknown]")
{
  using namespace boost::beast::http;

  SECTION("GET /unknown-path returns 404")
  {
    request<string_body> req{verb::get, "/unknown-path", 11};
    req.set(field::host, "localhost");
    req.set(field::user_agent, "test-client");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::not_found);
    REQUIRE(response[field::server] == "ByteBucket-Server");
    REQUIRE(response[field::content_type] == "text/plain");
    REQUIRE(response.body() == "Not found");
  }

  SECTION("POST /random-endpoint returns 404")
  {
    request<string_body> req{verb::post, "/random-endpoint", 11};
    req.set(field::host, "localhost");
    req.body() = "some data";
    req.prepare_payload();

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::not_found);
    REQUIRE(response.body() == "Not found");
  }

  SECTION("Various malformed paths return 404")
  {
    std::vector<std::string> malformed_paths = {
        "//double-slash",
        "/download",     // Missing file ID
        "/upload/extra", // Extra path segment
        "/health/extra", // Extra path segment
        "no-leading-slash"};

    for (const auto &path : malformed_paths)
    {
      request<string_body> req{verb::get, path, 11};
      req.set(field::host, "localhost");

      auto response = bytebucket::test::handle_request_direct(std::move(req));

      REQUIRE(response.result() == status::not_found);
      REQUIRE(response.body() == "Not found");
    }
  }
}
