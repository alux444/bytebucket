#include <catch2/catch_test_macros.hpp>
#include <boost/beast/http.hpp>
#include "request_handler.hpp"
#include "test_helpers.hpp"

TEST_CASE("Health endpoint tests", "[health]")
{
  using namespace boost::beast::http;

  SECTION("GET /health returns ok status")
  {
    request<string_body> req{verb::get, "/health", 11};
    req.set(field::host, "localhost");
    req.set(field::user_agent, "test-client");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::ok);
    REQUIRE(response[field::server] == "ByteBucket-Server");
    REQUIRE(response[field::content_type] == "application/json");
    REQUIRE(response.body() == R"({"status":"ok"})");
  }

  SECTION("POST /health should return 404")
  {
    request<string_body> req{verb::post, "/health", 11};
    req.set(field::host, "localhost");
    req.set(field::user_agent, "test-client");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::not_found);
  }

  SECTION("GET /health with different HTTP versions")
  {
    request<string_body> req{verb::get, "/health", 10};
    req.set(field::host, "localhost");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::ok);
    REQUIRE(response.version() == 10);
  }
}
