#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <vector>
#include "file_storage.hpp"

TEST_CASE("FileStorage tests", "[file_storage]")
{
  using namespace bytebucket;

  SECTION("Save and retrieve file successfully")
  {
    std::string filename = "test_file.txt";
    std::string content = "Hello, World!";
    std::vector<char> file_content(content.begin(), content.end());
    std::string content_type = "text/plain";

    auto file_id = FileStorage::saveFile(filename, file_content, content_type);

    REQUIRE(file_id.has_value());
    REQUIRE(!file_id->empty());

    // Check if file exists
    REQUIRE(FileStorage::fileExists(*file_id));

    // Get file path and verify content
    auto file_path = FileStorage::getFilePath(*file_id);
    REQUIRE(file_path.has_value());
    REQUIRE(std::filesystem::exists(*file_path));

    // Read and verify file content
    std::ifstream file(*file_path, std::ios::binary);
    std::string read_content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
    REQUIRE(read_content == content);

    // Verify metadata file exists
    std::filesystem::path metadata_path = file_path->string() + ".meta";
    REQUIRE(std::filesystem::exists(metadata_path));

    // Read and verify metadata
    std::ifstream metadata(metadata_path);
    std::string metadata_content((std::istreambuf_iterator<char>(metadata)),
                                 std::istreambuf_iterator<char>());

    REQUIRE(metadata_content.find("original_filename=" + filename) != std::string::npos);
    REQUIRE(metadata_content.find("content_type=" + content_type) != std::string::npos);
    REQUIRE(metadata_content.find("size=" + std::to_string(file_content.size())) != std::string::npos);
    REQUIRE(metadata_content.find("uploaded_at=") != std::string::npos);

    // Cleanup
    std::filesystem::remove(*file_path);
    std::filesystem::remove(metadata_path);
  }

  SECTION("Save binary file")
  {
    std::string filename = "test_binary.bin";
    std::vector<unsigned char> binary_content = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0xFD};
    std::string content_type = "application/octet-stream";

    auto file_id = FileStorage::saveFile(filename, std::vector<char>(binary_content.begin(), binary_content.end()), content_type);

    REQUIRE(file_id.has_value());

    auto file_path = FileStorage::getFilePath(*file_id);
    REQUIRE(file_path.has_value());

    // Read binary content
    std::ifstream file(*file_path, std::ios::binary);
    std::vector<unsigned char> read_content((std::istreambuf_iterator<char>(file)),
                                            std::istreambuf_iterator<char>());

    REQUIRE(read_content.size() == binary_content.size());
    REQUIRE(std::equal(read_content.begin(), read_content.end(), binary_content.begin()));

    // Cleanup
    std::filesystem::remove(*file_path);
    std::filesystem::remove(file_path->string() + ".meta");
  }

  SECTION("Save empty file")
  {
    std::string filename = "empty_file.txt";
    std::vector<char> empty_content;
    std::string content_type = "text/plain";

    auto file_id = FileStorage::saveFile(filename, empty_content, content_type);

    REQUIRE(file_id.has_value());
    REQUIRE(FileStorage::fileExists(*file_id));

    auto file_path = FileStorage::getFilePath(*file_id);
    REQUIRE(file_path.has_value());
    REQUIRE(std::filesystem::file_size(*file_path) == 0);

    // Cleanup
    std::filesystem::remove(*file_path);
    std::filesystem::remove(file_path->string() + ".meta");
  }

  SECTION("File ID uniqueness")
  {
    std::string filename = "test_unique.txt";
    std::vector<char> content = {'t', 'e', 's', 't'};
    std::string content_type = "text/plain";

    auto file_id1 = FileStorage::saveFile(filename, content, content_type);
    auto file_id2 = FileStorage::saveFile(filename, content, content_type);

    REQUIRE(file_id1.has_value());
    REQUIRE(file_id2.has_value());
    REQUIRE(*file_id1 != *file_id2);

    // Cleanup
    auto path1 = FileStorage::getFilePath(*file_id1);
    auto path2 = FileStorage::getFilePath(*file_id2);
    if (path1)
    {
      std::filesystem::remove(*path1);
      std::filesystem::remove(path1->string() + ".meta");
    }
    if (path2)
    {
      std::filesystem::remove(*path2);
      std::filesystem::remove(path2->string() + ".meta");
    }
  }

  SECTION("Non-existent file operations")
  {
    std::string fake_id = "nonexistent_file_id";

    REQUIRE_FALSE(FileStorage::fileExists(fake_id));

    auto file_path = FileStorage::getFilePath(fake_id);
    REQUIRE_FALSE(file_path.has_value());
  }

  SECTION("Large file handling")
  {
    std::string filename = "large_file.bin";
    std::vector<char> large_content(10000, 'A'); // 10KB of 'A's
    std::string content_type = "application/octet-stream";

    auto file_id = FileStorage::saveFile(filename, large_content, content_type);

    REQUIRE(file_id.has_value());

    auto file_path = FileStorage::getFilePath(*file_id);
    REQUIRE(file_path.has_value());
    REQUIRE(std::filesystem::file_size(*file_path) == large_content.size());

    // Cleanup
    std::filesystem::remove(*file_path);
    std::filesystem::remove(file_path->string() + ".meta");
  }
}