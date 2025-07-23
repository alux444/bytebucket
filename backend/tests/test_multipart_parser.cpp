#include <catch2/catch_test_macros.hpp>
#include "multipart_parser.hpp"

TEST_CASE("MultipartParser tests", "[multipart_parser]")
{
  using namespace bytebucket;

  SECTION("Extract boundary from Content-Type")
  {
    std::string content_type1 = "multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string boundary1 = MultipartParser::extractBoundary(content_type1);
    REQUIRE(boundary1 == "----WebKitFormBoundary7MA4YWxkTrZu0gW");

    std::string content_type2 = "multipart/form-data; boundary=simple";
    std::string boundary2 = MultipartParser::extractBoundary(content_type2);
    REQUIRE(boundary2 == "simple");

    std::string content_type3 = "multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW; charset=utf-8";
    std::string boundary3 = MultipartParser::extractBoundary(content_type3);
    REQUIRE(boundary3 == "----WebKitFormBoundary7MA4YWxkTrZu0gW");

    std::string content_type4 = "application/json";
    std::string boundary4 = MultipartParser::extractBoundary(content_type4);
    REQUIRE(boundary4.empty());
  }

  SECTION("Parse simple form field")
  {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string body =
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"username\"\r\n"
        "\r\n"
        "john_doe\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    auto result = MultipartParser::parse(body, boundary);

    REQUIRE(result.has_value());
    REQUIRE(result->fields.size() == 1);
    REQUIRE(result->files.size() == 0);

    REQUIRE(result->fields[0].name == "username");
    REQUIRE(result->fields[0].value == "john_doe");
  }

  SECTION("Parse multiple form fields")
  {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string body =
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"username\"\r\n"
        "\r\n"
        "john_doe\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"email\"\r\n"
        "\r\n"
        "john@example.com\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"description\"\r\n"
        "\r\n"
        "This is a test description\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    auto result = MultipartParser::parse(body, boundary);

    REQUIRE(result.has_value());
    REQUIRE(result->fields.size() == 3);
    REQUIRE(result->files.size() == 0);

    REQUIRE(result->fields[0].name == "username");
    REQUIRE(result->fields[0].value == "john_doe");
    REQUIRE(result->fields[1].name == "email");
    REQUIRE(result->fields[1].value == "john@example.com");
    REQUIRE(result->fields[2].name == "description");
    REQUIRE(result->fields[2].value == "This is a test description");
  }

  SECTION("Parse file upload")
  {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string body =
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello, World!\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    auto result = MultipartParser::parse(body, boundary);

    REQUIRE(result.has_value());
    REQUIRE(result->fields.size() == 0);
    REQUIRE(result->files.size() == 1);

    REQUIRE(result->files[0].name == "file");
    REQUIRE(result->files[0].filename == "test.txt");
    REQUIRE(result->files[0].content_type == "text/plain");

    std::string file_content(result->files[0].content.begin(), result->files[0].content.end());
    REQUIRE(file_content == "Hello, World!");
  }

  SECTION("Parse mixed form fields and files")
  {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string body =
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"username\"\r\n"
        "\r\n"
        "john_doe\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"avatar\"; filename=\"profile.jpg\"\r\n"
        "Content-Type: image/jpeg\r\n"
        "\r\n"
        "fake_jpeg_data\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"description\"\r\n"
        "\r\n"
        "My profile picture\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    auto result = MultipartParser::parse(body, boundary);

    REQUIRE(result.has_value());
    REQUIRE(result->fields.size() == 2);
    REQUIRE(result->files.size() == 1);

    // Check fields
    REQUIRE(result->fields[0].name == "username");
    REQUIRE(result->fields[0].value == "john_doe");
    REQUIRE(result->fields[1].name == "description");
    REQUIRE(result->fields[1].value == "My profile picture");

    // Check file
    REQUIRE(result->files[0].name == "avatar");
    REQUIRE(result->files[0].filename == "profile.jpg");
    REQUIRE(result->files[0].content_type == "image/jpeg");

    std::string file_content(result->files[0].content.begin(), result->files[0].content.end());
    REQUIRE(file_content == "fake_jpeg_data");
  }

  SECTION("Parse file without explicit Content-Type")
  {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string body =
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"data.bin\"\r\n"
        "\r\n"
        "binary_data_here\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    auto result = MultipartParser::parse(body, boundary);

    REQUIRE(result.has_value());
    REQUIRE(result->files.size() == 1);

    REQUIRE(result->files[0].content_type == "application/octet-stream");
  }

  SECTION("Parse empty form field")
  {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string body =
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"empty_field\"\r\n"
        "\r\n"
        "\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    auto result = MultipartParser::parse(body, boundary);

    REQUIRE(result.has_value());
    REQUIRE(result->fields.size() == 1);
    REQUIRE(result->fields[0].name == "empty_field");
    REQUIRE(result->fields[0].value.empty());
  }

  SECTION("Parse empty file")
  {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string body =
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"empty.txt\"\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    auto result = MultipartParser::parse(body, boundary);

    REQUIRE(result.has_value());
    REQUIRE(result->files.size() == 1);
    REQUIRE(result->files[0].content.empty());
  }

  SECTION("Handle malformed parts gracefully")
  {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string body =
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data\r\n" // Missing name attribute
        "\r\n"
        "should_be_ignored\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"valid_field\"\r\n"
        "\r\n"
        "valid_value\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    auto result = MultipartParser::parse(body, boundary);

    REQUIRE(result.has_value());
    REQUIRE(result->fields.size() == 1);
    REQUIRE(result->fields[0].name == "valid_field");
    REQUIRE(result->fields[0].value == "valid_value");
  }

  SECTION("Handle empty boundary")
  {
    std::string body = "some content";
    std::string empty_boundary = "";

    auto result = MultipartParser::parse(body, empty_boundary);

    REQUIRE_FALSE(result.has_value());
  }

  SECTION("Handle binary file content")
  {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string binary_content = std::string("\x00\x01\x02\xFF\xFE\xFD", 6);
    std::string body =
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"binary_file\"; filename=\"test.bin\"\r\n"
        "Content-Type: application/octet-stream\r\n"
        "\r\n" +
        binary_content + "\r\n"
                         "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    auto result = MultipartParser::parse(body, boundary);

    REQUIRE(result.has_value());
    REQUIRE(result->files.size() == 1);
    REQUIRE(result->files[0].content.size() == 6);

    std::vector<char> expected_content(binary_content.begin(), binary_content.end());
    REQUIRE(std::equal(result->files[0].content.begin(), result->files[0].content.end(),
                       expected_content.begin()));
  }

  SECTION("Trim utility function")
  {
    REQUIRE(MultipartParser::trim("  hello  ") == "hello");
    REQUIRE(MultipartParser::trim("\t\r\ntest\t\r\n") == "test");
    REQUIRE(MultipartParser::trim("") == "");
    REQUIRE(MultipartParser::trim("   ") == "");
    REQUIRE(MultipartParser::trim("no_spaces") == "no_spaces");
  }
}
