#include <catch2/catch_test_macros.hpp>
#include "test_helpers_database.hpp"
#include <set>
#include <algorithm>

using namespace bytebucket;
using namespace bytebucket::test;
using TestDatabase = DatabaseTestHelper::TestDatabase;

TEST_CASE("Database metadata operations - Set metadata", "[database][metadata][set]")
{
  TestDatabase test_db("metadata_set");

  // Set up test data
  auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "MetadataTestFolder");
  auto file_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "test_file.txt", 100, "text/plain", "storage_metadata");

  SECTION("Set metadata successfully")
  {
    auto result = test_db->setFileMetadata(file_id.value(), "author", "John Doe");

    REQUIRE(result.success());
    REQUIRE(result.value.value() == true);
  }

  SECTION("Set multiple metadata entries for same file")
  {
    auto result1 = test_db->setFileMetadata(file_id.value(), "author", "John Doe");
    auto result2 = test_db->setFileMetadata(file_id.value(), "title", "My Document");
    auto result3 = test_db->setFileMetadata(file_id.value(), "version", "1.0");

    REQUIRE(result1.success());
    REQUIRE(result2.success());
    REQUIRE(result3.success());
  }

  SECTION("Update existing metadata (UPSERT behavior)")
  {
    // Set initial value
    auto result1 = test_db->setFileMetadata(file_id.value(), "version", "1.0");
    REQUIRE(result1.success());

    // Update to new value
    auto result2 = test_db->setFileMetadata(file_id.value(), "version", "2.0");
    REQUIRE(result2.success());

    // Verify updated value
    auto get_result = test_db->getFileMetadata(file_id.value(), "version");
    REQUIRE(get_result.success());
    REQUIRE(get_result.value.value() == "2.0");
  }

  SECTION("Set metadata with empty value")
  {
    auto result = test_db->setFileMetadata(file_id.value(), "description", "");

    REQUIRE(result.success());

    // Verify empty value is stored
    auto get_result = test_db->getFileMetadata(file_id.value(), "description");
    REQUIRE(get_result.success());
    REQUIRE(get_result.value.value() == "");
  }

  SECTION("Set metadata with special characters")
  {
    auto result1 = test_db->setFileMetadata(file_id.value(), "tag with spaces", "value with spaces & symbols!");
    auto result2 = test_db->setFileMetadata(file_id.value(), "unicode-key", "тест значение");

    REQUIRE(result1.success());
    REQUIRE(result2.success());

    // Verify retrieval
    auto get1 = test_db->getFileMetadata(file_id.value(), "tag with spaces");
    auto get2 = test_db->getFileMetadata(file_id.value(), "unicode-key");

    REQUIRE(get1.success());
    REQUIRE(get2.success());
    REQUIRE(get1.value.value() == "value with spaces & symbols!");
    REQUIRE(get2.value.value() == "тест значение");
  }

  SECTION("Set metadata with very long key and value")
  {
    std::string long_key(500, 'k');
    std::string long_value(1000, 'v');

    auto result = test_db->setFileMetadata(file_id.value(), long_key, long_value);

    REQUIRE(result.success());

    // Verify retrieval
    auto get_result = test_db->getFileMetadata(file_id.value(), long_key);
    REQUIRE(get_result.success());
    REQUIRE(get_result.value.value() == long_value);
  }

  SECTION("Set metadata with empty key should fail")
  {
    auto result = test_db->setFileMetadata(file_id.value(), "", "some value");

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::NotNullConstraint);
    REQUIRE(result.errorMessage == "Metadata key cannot be empty");
  }

  SECTION("Set metadata for non-existent file should fail")
  {
    auto result = test_db->setFileMetadata(99999, "author", "John Doe");

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::ForeignKeyConstraint);
    REQUIRE(result.errorMessage == "File doesn't exist");
  }

  SECTION("Set metadata with whitespace-only key")
  {
    // These should succeed since we only check for completely empty keys
    auto result1 = test_db->setFileMetadata(file_id.value(), " ", "space key");
    auto result2 = test_db->setFileMetadata(file_id.value(), "\t", "tab key");
    auto result3 = test_db->setFileMetadata(file_id.value(), "\n", "newline key");

    REQUIRE(result1.success());
    REQUIRE(result2.success());
    REQUIRE(result3.success());
  }

  SECTION("Set many metadata entries")
  {
    for (int i = 0; i < 100; ++i)
    {
      std::string key = "key_" + std::to_string(i);
      std::string value = "value_" + std::to_string(i);

      auto result = test_db->setFileMetadata(file_id.value(), key, value);
      REQUIRE(result.success());
    }

    // Verify all entries exist
    auto all_metadata = test_db->getAllFileMetadata(file_id.value());
    REQUIRE(all_metadata.success());
    REQUIRE(all_metadata.value.value().size() == 100);
  }
}

TEST_CASE("Database metadata operations - Get metadata", "[database][metadata][get]")
{
  TestDatabase test_db("metadata_get");

  // Set up test data
  auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "GetMetadataFolder");
  auto file_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "test_file.txt", 100, "text/plain", "storage_get_metadata");

  SECTION("Get existing metadata")
  {
    auto set_result = test_db->setFileMetadata(file_id.value(), "author", "Jane Smith");
    REQUIRE(set_result.success());

    auto get_result = test_db->getFileMetadata(file_id.value(), "author");
    REQUIRE(get_result.success());
    REQUIRE(get_result.value.value() == "Jane Smith");
  }

  SECTION("Get non-existent metadata should fail")
  {
    auto result = test_db->getFileMetadata(file_id.value(), "non_existent_key");

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::UnknownError);
    REQUIRE(result.errorMessage == "Metadata not found");
  }

  SECTION("Get metadata with empty key should fail")
  {
    auto result = test_db->getFileMetadata(file_id.value(), "");

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::UnknownError);
    REQUIRE(result.errorMessage == "Metadata key cannot be empty");
  }

  SECTION("Get metadata for non-existent file")
  {
    auto result = test_db->getFileMetadata(99999, "author");

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::UnknownError);
    REQUIRE(result.errorMessage == "Metadata not found");
  }

  SECTION("Get metadata with empty value")
  {
    auto set_result = test_db->setFileMetadata(file_id.value(), "empty_field", "");
    REQUIRE(set_result.success());

    auto get_result = test_db->getFileMetadata(file_id.value(), "empty_field");
    REQUIRE(get_result.success());
    REQUIRE(get_result.value.value() == "");
  }

  SECTION("Get metadata with special characters")
  {
    auto set_result = test_db->setFileMetadata(file_id.value(), "special-chars", "value with 特殊字符 & symbols!");
    REQUIRE(set_result.success());

    auto get_result = test_db->getFileMetadata(file_id.value(), "special-chars");
    REQUIRE(get_result.success());
    REQUIRE(get_result.value.value() == "value with 特殊字符 & symbols!");
  }

  SECTION("Case sensitive key lookup")
  {
    auto set1 = test_db->setFileMetadata(file_id.value(), "Author", "John");
    auto set2 = test_db->setFileMetadata(file_id.value(), "author", "Jane");
    auto set3 = test_db->setFileMetadata(file_id.value(), "AUTHOR", "Bob");

    REQUIRE(set1.success());
    REQUIRE(set2.success());
    REQUIRE(set3.success());

    // Should get correct values for each case
    auto get1 = test_db->getFileMetadata(file_id.value(), "Author");
    auto get2 = test_db->getFileMetadata(file_id.value(), "author");
    auto get3 = test_db->getFileMetadata(file_id.value(), "AUTHOR");

    REQUIRE(get1.success());
    REQUIRE(get2.success());
    REQUIRE(get3.success());

    REQUIRE(get1.value.value() == "John");
    REQUIRE(get2.value.value() == "Jane");
    REQUIRE(get3.value.value() == "Bob");
  }

  SECTION("Get updated metadata after multiple updates")
  {
    // Set initial value
    auto set1 = test_db->setFileMetadata(file_id.value(), "status", "draft");
    REQUIRE(set1.success());

    auto get1 = test_db->getFileMetadata(file_id.value(), "status");
    REQUIRE(get1.success());
    REQUIRE(get1.value.value() == "draft");

    // Update value
    auto set2 = test_db->setFileMetadata(file_id.value(), "status", "review");
    REQUIRE(set2.success());

    auto get2 = test_db->getFileMetadata(file_id.value(), "status");
    REQUIRE(get2.success());
    REQUIRE(get2.value.value() == "review");

    // Final update
    auto set3 = test_db->setFileMetadata(file_id.value(), "status", "published");
    REQUIRE(set3.success());

    auto get3 = test_db->getFileMetadata(file_id.value(), "status");
    REQUIRE(get3.success());
    REQUIRE(get3.value.value() == "published");
  }
}

TEST_CASE("Database metadata operations - Get all metadata", "[database][metadata][get_all]")
{
  TestDatabase test_db("metadata_get_all");

  // Set up test data
  auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "GetAllMetadataFolder");
  auto file_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "test_file.txt", 100, "text/plain", "storage_get_all");

  SECTION("Get all metadata for file with no metadata")
  {
    auto result = test_db->getAllFileMetadata(file_id.value());

    REQUIRE(result.success());
    REQUIRE(result.value.value().empty());
  }

  SECTION("Get all metadata for file with single entry")
  {
    auto set_result = test_db->setFileMetadata(file_id.value(), "author", "Alice");
    REQUIRE(set_result.success());

    auto get_result = test_db->getAllFileMetadata(file_id.value());
    REQUIRE(get_result.success());

    auto metadata = get_result.value.value();
    REQUIRE(metadata.size() == 1);
    REQUIRE(metadata[0].first == "author");
    REQUIRE(metadata[0].second == "Alice");
  }

  SECTION("Get all metadata for file with multiple entries - alphabetical order")
  {
    // Set metadata in random order
    auto set1 = test_db->setFileMetadata(file_id.value(), "zzz_last", "should be last");
    auto set2 = test_db->setFileMetadata(file_id.value(), "author", "middle entry");
    auto set3 = test_db->setFileMetadata(file_id.value(), "aaa_first", "should be first");

    REQUIRE(set1.success());
    REQUIRE(set2.success());
    REQUIRE(set3.success());

    auto get_result = test_db->getAllFileMetadata(file_id.value());
    REQUIRE(get_result.success());

    auto metadata = get_result.value.value();
    REQUIRE(metadata.size() == 3);

    // Should be returned in alphabetical order by key
    REQUIRE(metadata[0].first == "aaa_first");
    REQUIRE(metadata[0].second == "should be first");
    REQUIRE(metadata[1].first == "author");
    REQUIRE(metadata[1].second == "middle entry");
    REQUIRE(metadata[2].first == "zzz_last");
    REQUIRE(metadata[2].second == "should be last");
  }

  SECTION("Get all metadata for non-existent file")
  {
    auto result = test_db->getAllFileMetadata(99999);

    REQUIRE(result.success());
    REQUIRE(result.value.value().empty()); // Returns empty vector, not error
  }

  SECTION("Get all metadata with various data types")
  {
    auto set1 = test_db->setFileMetadata(file_id.value(), "created_date", "2024-08-16");
    auto set2 = test_db->setFileMetadata(file_id.value(), "file_size", "1024");
    auto set3 = test_db->setFileMetadata(file_id.value(), "is_public", "true");
    auto set4 = test_db->setFileMetadata(file_id.value(), "tags", "document,important,draft");
    auto set5 = test_db->setFileMetadata(file_id.value(), "description", "");

    REQUIRE(set1.success());
    REQUIRE(set2.success());
    REQUIRE(set3.success());
    REQUIRE(set4.success());
    REQUIRE(set5.success());

    auto get_result = test_db->getAllFileMetadata(file_id.value());
    REQUIRE(get_result.success());

    auto metadata = get_result.value.value();
    REQUIRE(metadata.size() == 5);

    // Verify alphabetical ordering and values
    REQUIRE(metadata[0].first == "created_date");
    REQUIRE(metadata[0].second == "2024-08-16");
    REQUIRE(metadata[1].first == "description");
    REQUIRE(metadata[1].second == "");
    REQUIRE(metadata[2].first == "file_size");
    REQUIRE(metadata[2].second == "1024");
    REQUIRE(metadata[3].first == "is_public");
    REQUIRE(metadata[3].second == "true");
    REQUIRE(metadata[4].first == "tags");
    REQUIRE(metadata[4].second == "document,important,draft");
  }

  SECTION("Get all metadata after updates and additions")
  {
    // Initial metadata
    auto set1 = test_db->setFileMetadata(file_id.value(), "version", "1.0");
    auto set2 = test_db->setFileMetadata(file_id.value(), "author", "Alice");

    REQUIRE(set1.success());
    REQUIRE(set2.success());

    // Update existing and add new
    auto update1 = test_db->setFileMetadata(file_id.value(), "version", "2.0");
    auto add1 = test_db->setFileMetadata(file_id.value(), "title", "Updated Document");

    REQUIRE(update1.success());
    REQUIRE(add1.success());

    auto get_result = test_db->getAllFileMetadata(file_id.value());
    REQUIRE(get_result.success());

    auto metadata = get_result.value.value();
    REQUIRE(metadata.size() == 3);

    // Check final state
    REQUIRE(metadata[0].first == "author");
    REQUIRE(metadata[0].second == "Alice");
    REQUIRE(metadata[1].first == "title");
    REQUIRE(metadata[1].second == "Updated Document");
    REQUIRE(metadata[2].first == "version");
    REQUIRE(metadata[2].second == "2.0"); // Updated value
  }

  SECTION("Get all metadata for multiple files")
  {
    auto file2_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "file2.txt", 200, "text/plain", "storage_file2");

    // Add different metadata to each file
    auto set1_file1 = test_db->setFileMetadata(file_id.value(), "author", "Alice");
    auto set2_file1 = test_db->setFileMetadata(file_id.value(), "type", "document");
    auto set1_file2 = test_db->setFileMetadata(file2_id.value(), "category", "image");

    REQUIRE(set1_file1.success());
    REQUIRE(set2_file1.success());
    REQUIRE(set1_file2.success());

    // Get metadata for first file
    auto get1_result = test_db->getAllFileMetadata(file_id.value());
    REQUIRE(get1_result.success());
    auto metadata1 = get1_result.value.value();
    REQUIRE(metadata1.size() == 2);
    REQUIRE(metadata1[0].first == "author");
    REQUIRE(metadata1[1].first == "type");

    // Get metadata for second file
    auto get2_result = test_db->getAllFileMetadata(file2_id.value());
    REQUIRE(get2_result.success());
    auto metadata2 = get2_result.value.value();
    REQUIRE(metadata2.size() == 1);
    REQUIRE(metadata2[0].first == "category");
  }

  SECTION("Get all metadata with large dataset")
  {
    // Add 50 metadata entries
    for (int i = 0; i < 50; ++i)
    {
      std::string key = "key_" + std::to_string(i);
      std::string value = "value_" + std::to_string(i);

      auto result = test_db->setFileMetadata(file_id.value(), key, value);
      REQUIRE(result.success());
    }

    auto get_result = test_db->getAllFileMetadata(file_id.value());
    REQUIRE(get_result.success());

    auto metadata = get_result.value.value();
    REQUIRE(metadata.size() == 50);

    // Verify alphabetical ordering
    for (size_t i = 1; i < metadata.size(); ++i)
    {
      REQUIRE(metadata[i - 1].first < metadata[i].first);
    }

    // Verify some specific entries
    REQUIRE(metadata[0].first == "key_0");
    REQUIRE(metadata[0].second == "value_0");
  }
}

TEST_CASE("Database metadata operations - Complex scenarios", "[database][metadata][complex]")
{
  TestDatabase test_db("metadata_complex");

  SECTION("Metadata workflow: set, get, update, get all")
  {
    auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "WorkflowFolder");
    auto file_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "workflow_file.txt", 100, "text/plain", "storage_workflow");

    // Set initial metadata
    auto set_result = test_db->setFileMetadata(file_id.value(), "document_type", "article");
    REQUIRE(set_result.success());

    // Get the metadata
    auto get_result = test_db->getFileMetadata(file_id.value(), "document_type");
    REQUIRE(get_result.success());
    REQUIRE(get_result.value.value() == "article");

    // Update the metadata
    auto update_result = test_db->setFileMetadata(file_id.value(), "document_type", "research_paper");
    REQUIRE(update_result.success());

    // Verify update
    auto get_updated = test_db->getFileMetadata(file_id.value(), "document_type");
    REQUIRE(get_updated.success());
    REQUIRE(get_updated.value.value() == "research_paper");

    // Add more metadata
    auto add1 = test_db->setFileMetadata(file_id.value(), "author", "Dr. Smith");
    auto add2 = test_db->setFileMetadata(file_id.value(), "published", "2024");
    REQUIRE(add1.success());
    REQUIRE(add2.success());

    // Get all metadata
    auto all_result = test_db->getAllFileMetadata(file_id.value());
    REQUIRE(all_result.success());
    auto all_metadata = all_result.value.value();
    REQUIRE(all_metadata.size() == 3);

    // Verify all entries in alphabetical order
    REQUIRE(all_metadata[0].first == "author");
    REQUIRE(all_metadata[0].second == "Dr. Smith");
    REQUIRE(all_metadata[1].first == "document_type");
    REQUIRE(all_metadata[1].second == "research_paper");
    REQUIRE(all_metadata[2].first == "published");
    REQUIRE(all_metadata[2].second == "2024");
  }

  SECTION("Metadata with file deletion cascade")
  {
    auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "CascadeFolder");
    auto file_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "cascade_file.txt", 100, "text/plain", "storage_cascade");

    // Add metadata to file
    auto set1 = test_db->setFileMetadata(file_id.value(), "author", "Test Author");
    auto set2 = test_db->setFileMetadata(file_id.value(), "category", "Test Category");

    REQUIRE(set1.success());
    REQUIRE(set2.success());

    // Verify metadata exists
    auto all_before = test_db->getAllFileMetadata(file_id.value());
    REQUIRE(all_before.success());
    REQUIRE(all_before.value.value().size() == 2);

    // Delete the file
    auto delete_result = test_db->deleteFile(file_id.value());
    REQUIRE(delete_result.success());

    // Metadata should be automatically deleted due to CASCADE
    auto all_after = test_db->getAllFileMetadata(file_id.value());
    REQUIRE(all_after.success());
    REQUIRE(all_after.value.value().empty());

    // Individual metadata queries should return not found
    auto get_author = test_db->getFileMetadata(file_id.value(), "author");
    REQUIRE_FALSE(get_author.success());
    REQUIRE(get_author.errorMessage == "Metadata not found");
  }

  SECTION("Stress test: many metadata operations")
  {
    auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "StressFolder");
    auto file_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "stress_file.txt", 100, "text/plain", "storage_stress");

    // Add 200 metadata entries
    for (int i = 0; i < 200; ++i)
    {
      std::string key = "stress_key_" + std::to_string(i);
      std::string value = "stress_value_" + std::to_string(i);

      auto result = test_db->setFileMetadata(file_id.value(), key, value);
      REQUIRE(result.success());
    }

    // Update every 10th entry
    for (int i = 0; i < 200; i += 10)
    {
      std::string key = "stress_key_" + std::to_string(i);
      std::string value = "updated_value_" + std::to_string(i);

      auto result = test_db->setFileMetadata(file_id.value(), key, value);
      REQUIRE(result.success());
    }

    // Verify final state
    auto all_result = test_db->getAllFileMetadata(file_id.value());
    REQUIRE(all_result.success());
    auto all_metadata = all_result.value.value();
    REQUIRE(all_metadata.size() == 200);

    // Verify alphabetical ordering
    for (size_t i = 1; i < all_metadata.size(); ++i)
    {
      REQUIRE(all_metadata[i - 1].first < all_metadata[i].first);
    }

    // Verify some updated values
    auto get_0 = test_db->getFileMetadata(file_id.value(), "stress_key_0");
    auto get_10 = test_db->getFileMetadata(file_id.value(), "stress_key_10");
    auto get_5 = test_db->getFileMetadata(file_id.value(), "stress_key_5");

    REQUIRE(get_0.success());
    REQUIRE(get_10.success());
    REQUIRE(get_5.success());

    REQUIRE(get_0.value.value() == "updated_value_0");
    REQUIRE(get_10.value.value() == "updated_value_10");
    REQUIRE(get_5.value.value() == "stress_value_5"); // Not updated
  }

  SECTION("Metadata across multiple files")
  {
    auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "MultiFileFolder");

    // Create multiple files
    std::vector<int> file_ids;
    for (int i = 0; i < 5; ++i)
    {
      std::string file_name = "file_" + std::to_string(i) + ".txt";
      std::string storage_id = "storage_multi_" + std::to_string(i);
      auto file_result = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), file_name, 100, "text/plain", storage_id);
      REQUIRE(file_result.has_value());
      file_ids.push_back(file_result.value());
    }

    // Add different metadata to each file
    for (size_t i = 0; i < file_ids.size(); ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        std::string key = "key_" + std::to_string(j);
        std::string value = "file" + std::to_string(i) + "_value" + std::to_string(j);

        auto result = test_db->setFileMetadata(file_ids[i], key, value);
        REQUIRE(result.success());
      }
    }

    // Verify each file has its own metadata
    for (size_t i = 0; i < file_ids.size(); ++i)
    {
      auto all_result = test_db->getAllFileMetadata(file_ids[i]);
      REQUIRE(all_result.success());
      auto metadata = all_result.value.value();
      REQUIRE(metadata.size() == 3);

      // Verify values are specific to this file
      for (const auto &[key, value] : metadata)
      {
        REQUIRE(value.find("file" + std::to_string(i)) != std::string::npos);
      }
    }
  }
}

TEST_CASE("Database metadata operations - Validation", "[database][metadata][validation]")
{
  TestDatabase test_db("metadata_validation");

  auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "ValidationFolder");
  auto file_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "validation_file.txt", 100, "text/plain", "storage_validation");

  SECTION("Empty key validation")
  {
    // Set with empty key should fail
    auto set_result = test_db->setFileMetadata(file_id.value(), "", "some value");
    REQUIRE_FALSE(set_result.success());
    REQUIRE(set_result.error == DatabaseError::NotNullConstraint);
    REQUIRE(set_result.errorMessage == "Metadata key cannot be empty");

    // Get with empty key should fail
    auto get_result = test_db->getFileMetadata(file_id.value(), "");
    REQUIRE_FALSE(get_result.success());
    REQUIRE(get_result.error == DatabaseError::UnknownError);
    REQUIRE(get_result.errorMessage == "Metadata key cannot be empty");
  }

  SECTION("Valid minimal keys")
  {
    // Single character keys should work
    auto result_a = test_db->setFileMetadata(file_id.value(), "a", "value_a");
    auto result_1 = test_db->setFileMetadata(file_id.value(), "1", "value_1");
    auto result_symbol = test_db->setFileMetadata(file_id.value(), "!", "value_!");

    REQUIRE(result_a.success());
    REQUIRE(result_1.success());
    REQUIRE(result_symbol.success());

    // Should be able to retrieve them
    auto get_a = test_db->getFileMetadata(file_id.value(), "a");
    auto get_1 = test_db->getFileMetadata(file_id.value(), "1");
    auto get_symbol = test_db->getFileMetadata(file_id.value(), "!");

    REQUIRE(get_a.success());
    REQUIRE(get_1.success());
    REQUIRE(get_symbol.success());

    REQUIRE(get_a.value.value() == "value_a");
    REQUIRE(get_1.value.value() == "value_1");
    REQUIRE(get_symbol.value.value() == "value_!");
  }

  SECTION("Foreign key constraint validation")
  {
    // Try to set metadata for non-existent file
    auto result = test_db->setFileMetadata(99999, "test_key", "test_value");

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::ForeignKeyConstraint);
    REQUIRE(result.errorMessage == "File doesn't exist");
  }

  SECTION("NULL value handling")
  {
    // Set empty string value
    auto set_result = test_db->setFileMetadata(file_id.value(), "nullable_field", "");
    REQUIRE(set_result.success());

    // Get should return empty string
    auto get_result = test_db->getFileMetadata(file_id.value(), "nullable_field");
    REQUIRE(get_result.success());
    REQUIRE(get_result.value.value() == "");

    // Should appear in getAllFileMetadata
    auto all_result = test_db->getAllFileMetadata(file_id.value());
    REQUIRE(all_result.success());
    auto metadata = all_result.value.value();

    bool found = false;
    for (const auto &[key, value] : metadata)
    {
      if (key == "nullable_field")
      {
        REQUIRE(value == "");
        found = true;
        break;
      }
    }
    REQUIRE(found);
  }
}
