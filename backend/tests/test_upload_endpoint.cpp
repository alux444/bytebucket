#include <catch2/catch_test_macros.hpp>
#include <boost/beast/http.hpp>
#include "request_handler.hpp"
#include "test_helpers.hpp"

TEST_CASE("Upload endpoint tests", "[upload]")
{
  using namespace boost::beast::http;

  SECTION("POST /upload with missing Content-Type header")
  {
    request<string_body> req{verb::post, "/upload", 11};
    req.set(field::host, "localhost");
    req.body() = "This is test file content";
    req.prepare_payload();

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::bad_request);
    REQUIRE(response[field::server] == "ByteBucket-Server");
    REQUIRE(response[field::content_type] == "application/json");
    REQUIRE(response.body() == R"({"error":"Content-Type header is required"})");
  }

  SECTION("POST /upload with non-multipart Content-Type")
  {
    request<string_body> req{verb::post, "/upload", 11};
    req.set(field::host, "localhost");
    req.set(field::content_type, "text/plain");
    req.body() = "This is test file content";
    req.prepare_payload();

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::bad_request);
    REQUIRE(response[field::server] == "ByteBucket-Server");
    REQUIRE(response[field::content_type] == "application/json");
    REQUIRE(response.body() == R"({"error":"Content-Type should be multipart/form-data"})");
  }

  SECTION("POST /upload with multipart/form-data but no boundary")
  {
    request<string_body> req{verb::post, "/upload", 11};
    req.set(field::host, "localhost");
    req.set(field::content_type, "multipart/form-data");
    req.body() = "This is test file content";
    req.prepare_payload();

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::bad_request);
    REQUIRE(response[field::server] == "ByteBucket-Server");
    REQUIRE(response[field::content_type] == "application/json");
    REQUIRE(response.body() == R"({"error":"Invalid boundary in Content-Type"})");
  }

  SECTION("POST /upload with valid multipart/form-data")
  {
    request<string_body> req{verb::post, "/upload", 11};
    req.set(field::host, "localhost");
    req.set(field::content_type, "multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW");

    std::string multipart_body =
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "This is test file content\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    req.body() = multipart_body;
    req.prepare_payload();

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::ok);
    REQUIRE(response[field::server] == "ByteBucket-Server");
    REQUIRE(response[field::content_type] == "application/octet-stream");
    REQUIRE(response[field::content_disposition] == "attachment; filename=\"uploaded_file\"");
    REQUIRE(response.body() == multipart_body);
  }

  SECTION("POST /upload with malformed multipart data")
  {
    request<string_body> req{verb::post, "/upload", 11};
    req.set(field::host, "localhost");
    req.set(field::content_type, "multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW");
    req.body() = "invalid multipart data";
    req.prepare_payload();

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::bad_request);
    REQUIRE(response[field::server] == "ByteBucket-Server");
    REQUIRE(response[field::content_type] == "application/json");
    REQUIRE(response.body() == R"({"error":"Failed to parse multipart data"})");
  }

  SECTION("GET /upload should return 404")
  {
    request<string_body> req{verb::get, "/upload", 11};
    req.set(field::host, "localhost");

    auto response = bytebucket::test::handle_request_direct(std::move(req));

    REQUIRE(response.result() == status::not_found);
    REQUIRE(response.body() == "Not found");
  }
}