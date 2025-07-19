#include <catch2/catch_test_macros.hpp>
#include <boost/beast/http.hpp>
#include "request_handler.hpp"
#include "test_helpers.hpp"

TEST_CASE("Root endpoint tests", "[root]")
{
  using namespace boost::beast::http;

  SECTION("GET / returns ByteBucket message")
  {
    request<string_body> req{verb::get, "/", 11};
    req.set(field::host, "localhost");
    req.set(field::user_agent, "test-client");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::ok);
    REQUIRE(response[field::server] == "ByteBucket-Server");
    REQUIRE(response[field::content_type] == "text/plain");
    REQUIRE(response.body() == "ByteBucket");
  }

  SECTION("POST / should return 404")
  {
    request<string_body> req{verb::post, "/", 11};
    req.set(field::host, "localhost");
    req.set(field::user_agent, "test-client");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::not_found);
    REQUIRE(response.body() == "Not found");
  }

  SECTION("PUT / should return 404")
  {
    request<string_body> req{verb::put, "/", 11};
    req.set(field::host, "localhost");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::not_found);
  }

  SECTION("DELETE / should return 404")
  {
    request<string_body> req{verb::delete_, "/", 11};
    req.set(field::host, "localhost");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::not_found);
  }
}
