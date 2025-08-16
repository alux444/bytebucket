#include <catch2/catch_test_macros.hpp>
#include "test_helpers_database.hpp"
#include <set>
#include <algorithm>

using namespace bytebucket;
using namespace bytebucket::test;
using TestDatabase = DatabaseTestHelper::TestDatabase;

TEST_CASE("Database tag operations - Insert", "[database][tags][insert]")
{
  TestDatabase test_db("tags_insert");

  SECTION("Insert tag successfully")
  {
    auto result = test_db->insertTag("document");

    REQUIRE(result.success());
    REQUIRE(result.value.has_value());
    REQUIRE(result.value.value() > 0);
  }

  SECTION("Insert multiple tags")
  {
    auto tag1_result = test_db->insertTag("document");
    auto tag2_result = test_db->insertTag("image");
    auto tag3_result = test_db->insertTag("video");

    REQUIRE(tag1_result.success());
    REQUIRE(tag2_result.success());
    REQUIRE(tag3_result.success());

    // All IDs should be different
    REQUIRE(tag1_result.value.value() != tag2_result.value.value());
    REQUIRE(tag2_result.value.value() != tag3_result.value.value());
    REQUIRE(tag1_result.value.value() != tag3_result.value.value());
  }

  SECTION("Insert tag with special characters in name")
  {
    auto result = test_db->insertTag("tag with spaces & symbols!");

    REQUIRE(result.success());
    REQUIRE(result.value.value() > 0);
  }

  SECTION("Insert tag with Unicode name")
  {
    auto result = test_db->insertTag("тег");

    REQUIRE(result.success());
    REQUIRE(result.value.value() > 0);
  }

  SECTION("Insert tag with empty name")
  {
    auto result = test_db->insertTag("");

    REQUIRE_FALSE(result.success()); // Empty name should not be allowed
    REQUIRE(result.error == DatabaseError::NotNullConstraint);
    REQUIRE(result.errorMessage == "Tag name cannot be empty");
  }

  SECTION("Insert tag with only whitespace")
  {
    // TODO: maybe one day change to not allow only whitespace chars
    auto result1 = test_db->insertTag(" ");
    auto result2 = test_db->insertTag("   ");
    auto result3 = test_db->insertTag("\t");
    auto result4 = test_db->insertTag("\n");

    REQUIRE(result1.success());
    REQUIRE(result2.success());
    REQUIRE(result3.success());
    REQUIRE(result4.success());
  }

  SECTION("Insert tag with very long name")
  {
    std::string long_name(1000, 'a'); // 1000 character name

    auto result = test_db->insertTag(long_name);

    REQUIRE(result.success());
  }

  SECTION("Duplicate tag name should fail")
  {
    auto tag1_result = test_db->insertTag("duplicate");
    REQUIRE(tag1_result.success());

    auto tag2_result = test_db->insertTag("duplicate");
    REQUIRE_FALSE(tag2_result.success());
    REQUIRE(tag2_result.error == DatabaseError::UniqueConstraint);
    REQUIRE(tag2_result.errorMessage == "A tag with this name already exists");
  }

  SECTION("Case sensitive tag names")
  {
    auto tag1_result = test_db->insertTag("Document");
    auto tag2_result = test_db->insertTag("document");
    auto tag3_result = test_db->insertTag("DOCUMENT");

    REQUIRE(tag1_result.success());
    REQUIRE(tag2_result.success());
    REQUIRE(tag3_result.success());

    // All should have different IDs (case sensitive)
    REQUIRE(tag1_result.value.value() != tag2_result.value.value());
    REQUIRE(tag2_result.value.value() != tag3_result.value.value());
    REQUIRE(tag1_result.value.value() != tag3_result.value.value());
  }

  SECTION("Returned tag ID is sequential")
  {
    auto tag1_result = test_db->insertTag("seq1");
    auto tag2_result = test_db->insertTag("seq2");
    auto tag3_result = test_db->insertTag("seq3");

    REQUIRE(tag1_result.success());
    REQUIRE(tag2_result.success());
    REQUIRE(tag3_result.success());

    // IDs should be sequential (assuming no other operations)
    REQUIRE(tag2_result.value.value() > tag1_result.value.value());
    REQUIRE(tag3_result.value.value() > tag2_result.value.value());
  }

  SECTION("Insert many tags quickly")
  {
    std::vector<int> tag_ids;

    for (int i = 0; i < 100; ++i)
    {
      std::string tag_name = "bulk_tag_" + std::to_string(i);
      auto result = test_db->insertTag(tag_name);
      REQUIRE(result.success());
      tag_ids.push_back(result.value.value());
    }

    // All IDs should be unique
    std::set<int> unique_ids(tag_ids.begin(), tag_ids.end());
    REQUIRE(unique_ids.size() == 100);
  }
}

TEST_CASE("Database tag operations - Get by name", "[database][tags][get]")
{
  TestDatabase test_db("tags_get");

  SECTION("Get existing tag by name")
  {
    auto insert_result = test_db->insertTag("test_tag");
    REQUIRE(insert_result.success());
    auto inserted_id = insert_result.value.value();

    auto get_result = test_db->getTagByName("test_tag");
    REQUIRE(get_result.success());
    REQUIRE(get_result.value.value() == inserted_id);
  }

  SECTION("Get non-existent tag by name should fail")
  {
    auto result = test_db->getTagByName("non_existent_tag");

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::UnknownError);
    REQUIRE(result.errorMessage == "Tag not found");
  }

  SECTION("Get tag with special characters")
  {
    auto insert_result = test_db->insertTag("tag with spaces & symbols!");
    REQUIRE(insert_result.success());

    auto get_result = test_db->getTagByName("tag with spaces & symbols!");
    REQUIRE(get_result.success());
    REQUIRE(get_result.value.value() == insert_result.value.value());
  }

  SECTION("Get tag with Unicode name")
  {
    auto insert_result = test_db->insertTag("тег");
    REQUIRE(insert_result.success());

    auto get_result = test_db->getTagByName("тег");
    REQUIRE(get_result.success());
    REQUIRE(get_result.value.value() == insert_result.value.value());
  }

  SECTION("Get tag with empty name")
  {
    auto get_result = test_db->getTagByName("");

    REQUIRE_FALSE(get_result.success()); // Empty name should not be allowed
    REQUIRE(get_result.error == DatabaseError::UnknownError);
    REQUIRE(get_result.errorMessage == "Tag name cannot be empty");
  }

  SECTION("Case sensitive tag lookup")
  {
    auto lower_result = test_db->insertTag("document");
    auto upper_result = test_db->insertTag("DOCUMENT");
    auto mixed_result = test_db->insertTag("Document");

    REQUIRE(lower_result.success());
    REQUIRE(upper_result.success());
    REQUIRE(mixed_result.success());

    // Should get correct IDs for each case
    auto get_lower = test_db->getTagByName("document");
    auto get_upper = test_db->getTagByName("DOCUMENT");
    auto get_mixed = test_db->getTagByName("Document");

    REQUIRE(get_lower.success());
    REQUIRE(get_upper.success());
    REQUIRE(get_mixed.success());

    REQUIRE(get_lower.value.value() == lower_result.value.value());
    REQUIRE(get_upper.value.value() == upper_result.value.value());
    REQUIRE(get_mixed.value.value() == mixed_result.value.value());
  }

  SECTION("Get multiple different tags")
  {
    std::vector<std::string> tag_names = {"document", "image", "video", "audio", "archive"};
    std::vector<int> inserted_ids;

    // Insert all tags
    for (const auto &name : tag_names)
    {
      auto insert_result = test_db->insertTag(name);
      REQUIRE(insert_result.success());
      inserted_ids.push_back(insert_result.value.value());
    }

    // Get all tags back
    for (size_t i = 0; i < tag_names.size(); ++i)
    {
      auto get_result = test_db->getTagByName(tag_names[i]);
      REQUIRE(get_result.success());
      REQUIRE(get_result.value.value() == inserted_ids[i]);
    }
  }
}

TEST_CASE("Database file tag operations - Add/Remove", "[database][tags][file_tags]")
{
  TestDatabase test_db("file_tags");

  // Set up test data
  auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "TagTestFolder");
  auto file_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "test_file.txt", 100, "text/plain", "storage_file_tag");

  auto tag1_result = test_db->insertTag("document");
  auto tag2_result = test_db->insertTag("important");
  auto tag3_result = test_db->insertTag("draft");

  REQUIRE(tag1_result.success());
  REQUIRE(tag2_result.success());
  REQUIRE(tag3_result.success());

  auto tag1_id = tag1_result.value.value();
  auto tag2_id = tag2_result.value.value();
  auto tag3_id = tag3_result.value.value();

  SECTION("Add tag to file successfully")
  {
    auto result = test_db->addFileTag(file_id.value(), tag1_id);

    REQUIRE(result.success());
    REQUIRE(result.value.value() == true);
  }

  SECTION("Add multiple tags to same file")
  {
    auto result1 = test_db->addFileTag(file_id.value(), tag1_id);
    auto result2 = test_db->addFileTag(file_id.value(), tag2_id);
    auto result3 = test_db->addFileTag(file_id.value(), tag3_id);

    REQUIRE(result1.success());
    REQUIRE(result2.success());
    REQUIRE(result3.success());
  }

  SECTION("Add same tag to file twice should fail")
  {
    auto result1 = test_db->addFileTag(file_id.value(), tag1_id);
    REQUIRE(result1.success());

    auto result2 = test_db->addFileTag(file_id.value(), tag1_id);
    REQUIRE_FALSE(result2.success());
    REQUIRE(result2.error == DatabaseError::UniqueConstraint);
    REQUIRE(result2.errorMessage == "File already has this tag");
  }

  SECTION("Add tag to non-existent file should fail")
  {
    auto result = test_db->addFileTag(99999, tag1_id);

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::ForeignKeyConstraint);
    REQUIRE(result.errorMessage == "File or tag doesn't exist");
  }

  SECTION("Add non-existent tag to file should fail")
  {
    auto result = test_db->addFileTag(file_id.value(), 99999);

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::ForeignKeyConstraint);
    REQUIRE(result.errorMessage == "File or tag doesn't exist");
  }

  SECTION("Remove tag from file successfully")
  {
    // First add the tag
    auto add_result = test_db->addFileTag(file_id.value(), tag1_id);
    REQUIRE(add_result.success());

    // Then remove it
    auto remove_result = test_db->removeFileTag(file_id.value(), tag1_id);
    REQUIRE(remove_result.success());
    REQUIRE(remove_result.value.value() == true);
  }

  SECTION("Remove non-existent tag association should fail")
  {
    auto result = test_db->removeFileTag(file_id.value(), tag1_id);

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::UnknownError);
    REQUIRE(result.errorMessage == "File tag association not found");
  }

  SECTION("Remove tag from non-existent file")
  {
    auto result = test_db->removeFileTag(99999, tag1_id);

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::UnknownError);
    REQUIRE(result.errorMessage == "File tag association not found");
  }

  SECTION("Add and remove multiple tags")
  {
    // Add all tags
    auto add1 = test_db->addFileTag(file_id.value(), tag1_id);
    auto add2 = test_db->addFileTag(file_id.value(), tag2_id);
    auto add3 = test_db->addFileTag(file_id.value(), tag3_id);

    REQUIRE(add1.success());
    REQUIRE(add2.success());
    REQUIRE(add3.success());

    // Remove middle tag
    auto remove2 = test_db->removeFileTag(file_id.value(), tag2_id);
    REQUIRE(remove2.success());

    // Try to remove the same tag again - should fail
    auto remove2_again = test_db->removeFileTag(file_id.value(), tag2_id);
    REQUIRE_FALSE(remove2_again.success());

    // Remove remaining tags
    auto remove1 = test_db->removeFileTag(file_id.value(), tag1_id);
    auto remove3 = test_db->removeFileTag(file_id.value(), tag3_id);

    REQUIRE(remove1.success());
    REQUIRE(remove3.success());
  }

  SECTION("Add same tag to multiple files")
  {
    auto file2_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "file2.txt", 200, "text/plain", "storage_file2");
    auto file3_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "file3.txt", 300, "text/plain", "storage_file3");

    auto result1 = test_db->addFileTag(file_id.value(), tag1_id);
    auto result2 = test_db->addFileTag(file2_id.value(), tag1_id);
    auto result3 = test_db->addFileTag(file3_id.value(), tag1_id);

    REQUIRE(result1.success());
    REQUIRE(result2.success());
    REQUIRE(result3.success());
  }
}

TEST_CASE("Database file tag operations - Get file tags", "[database][tags][get_file_tags]")
{
  TestDatabase test_db("get_file_tags");

  // Set up test data
  auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "GetTagsFolder");
  auto file_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "tagged_file.txt", 100, "text/plain", "storage_tagged");

  auto tag1_result = test_db->insertTag("zzz_document"); // Will be last alphabetically
  auto tag2_result = test_db->insertTag("important");
  auto tag3_result = test_db->insertTag("aaa_draft"); // Will be first alphabetically

  REQUIRE(tag1_result.success());
  REQUIRE(tag2_result.success());
  REQUIRE(tag3_result.success());

  auto tag1_id = tag1_result.value.value();
  auto tag2_id = tag2_result.value.value();
  auto tag3_id = tag3_result.value.value();

  SECTION("Get tags for file with no tags")
  {
    auto result = test_db->getFileTags(file_id.value());

    REQUIRE(result.success());
    REQUIRE(result.value.value().empty());
  }

  SECTION("Get tags for file with one tag")
  {
    auto add_result = test_db->addFileTag(file_id.value(), tag1_id);
    REQUIRE(add_result.success());

    auto get_result = test_db->getFileTags(file_id.value());
    REQUIRE(get_result.success());

    auto tags = get_result.value.value();
    REQUIRE(tags.size() == 1);
    REQUIRE(tags[0] == "zzz_document");
  }

  SECTION("Get tags for file with multiple tags - alphabetical order")
  {
    // Add tags in random order
    auto add2 = test_db->addFileTag(file_id.value(), tag2_id);
    auto add3 = test_db->addFileTag(file_id.value(), tag3_id);
    auto add1 = test_db->addFileTag(file_id.value(), tag1_id);

    REQUIRE(add1.success());
    REQUIRE(add2.success());
    REQUIRE(add3.success());

    auto get_result = test_db->getFileTags(file_id.value());
    REQUIRE(get_result.success());

    auto tags = get_result.value.value();
    REQUIRE(tags.size() == 3);

    // Should be returned in alphabetical order
    REQUIRE(tags[0] == "aaa_draft");
    REQUIRE(tags[1] == "important");
    REQUIRE(tags[2] == "zzz_document");
  }

  SECTION("Get tags for non-existent file")
  {
    auto result = test_db->getFileTags(99999);

    REQUIRE(result.success());
    REQUIRE(result.value.value().empty()); // Returns empty vector, not error
  }

  SECTION("Get tags after removing some tags")
  {
    // Add all tags
    auto add1 = test_db->addFileTag(file_id.value(), tag1_id);
    auto add2 = test_db->addFileTag(file_id.value(), tag2_id);
    auto add3 = test_db->addFileTag(file_id.value(), tag3_id);

    REQUIRE(add1.success());
    REQUIRE(add2.success());
    REQUIRE(add3.success());

    // Remove middle tag
    auto remove = test_db->removeFileTag(file_id.value(), tag2_id);
    REQUIRE(remove.success());

    auto get_result = test_db->getFileTags(file_id.value());
    REQUIRE(get_result.success());

    auto tags = get_result.value.value();
    REQUIRE(tags.size() == 2);
    REQUIRE(tags[0] == "aaa_draft");
    REQUIRE(tags[1] == "zzz_document");
  }

  SECTION("Get tags with special characters")
  {
    auto special_tag_result = test_db->insertTag("tag with spaces & symbols!");
    REQUIRE(special_tag_result.success());

    auto add_result = test_db->addFileTag(file_id.value(), special_tag_result.value.value());
    REQUIRE(add_result.success());

    auto get_result = test_db->getFileTags(file_id.value());
    REQUIRE(get_result.success());

    auto tags = get_result.value.value();
    REQUIRE(tags.size() == 1);
    REQUIRE(tags[0] == "tag with spaces & symbols!");
  }

  SECTION("Get tags with Unicode characters")
  {
    auto unicode_tag_result = test_db->insertTag("тег");
    REQUIRE(unicode_tag_result.success());

    auto add_result = test_db->addFileTag(file_id.value(), unicode_tag_result.value.value());
    REQUIRE(add_result.success());

    auto get_result = test_db->getFileTags(file_id.value());
    REQUIRE(get_result.success());

    auto tags = get_result.value.value();
    REQUIRE(tags.size() == 1);
    REQUIRE(tags[0] == "тег");
  }

  SECTION("Get tags for multiple files")
  {
    auto file2_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "file2.txt", 200, "text/plain", "storage_file2");

    // Add different tags to each file
    auto add1_to_file1 = test_db->addFileTag(file_id.value(), tag1_id);
    auto add2_to_file1 = test_db->addFileTag(file_id.value(), tag2_id);
    auto add3_to_file2 = test_db->addFileTag(file2_id.value(), tag3_id);

    REQUIRE(add1_to_file1.success());
    REQUIRE(add2_to_file1.success());
    REQUIRE(add3_to_file2.success());

    // Get tags for first file
    auto tags1_result = test_db->getFileTags(file_id.value());
    REQUIRE(tags1_result.success());
    auto tags1 = tags1_result.value.value();
    REQUIRE(tags1.size() == 2);
    REQUIRE(tags1[0] == "important");
    REQUIRE(tags1[1] == "zzz_document");

    // Get tags for second file
    auto tags2_result = test_db->getFileTags(file2_id.value());
    REQUIRE(tags2_result.success());
    auto tags2 = tags2_result.value.value();
    REQUIRE(tags2.size() == 1);
    REQUIRE(tags2[0] == "aaa_draft");
  }
}

TEST_CASE("Database tag operations - Validation", "[database][tags][validation]")
{
  TestDatabase test_db("tags_validation");

  SECTION("Empty tag name validation")
  {
    // Insert empty tag should fail
    auto insert_result = test_db->insertTag("");
    REQUIRE_FALSE(insert_result.success());
    REQUIRE(insert_result.error == DatabaseError::NotNullConstraint);
    REQUIRE(insert_result.errorMessage == "Tag name cannot be empty");

    // Get empty tag should fail
    auto get_result = test_db->getTagByName("");
    REQUIRE_FALSE(get_result.success());
    REQUIRE(get_result.error == DatabaseError::UnknownError);
    REQUIRE(get_result.errorMessage == "Tag name cannot be empty");
  }

  SECTION("Valid minimal tag names")
  {
    // Single character tags should work
    auto result_a = test_db->insertTag("a");
    auto result_1 = test_db->insertTag("1");
    auto result_symbol = test_db->insertTag("!");

    REQUIRE(result_a.success());
    REQUIRE(result_1.success());
    REQUIRE(result_symbol.success());

    // Should be able to retrieve them
    auto get_a = test_db->getTagByName("a");
    auto get_1 = test_db->getTagByName("1");
    auto get_symbol = test_db->getTagByName("!");

    REQUIRE(get_a.success());
    REQUIRE(get_1.success());
    REQUIRE(get_symbol.success());

    REQUIRE(get_a.value.value() == result_a.value.value());
    REQUIRE(get_1.value.value() == result_1.value.value());
    REQUIRE(get_symbol.value.value() == result_symbol.value.value());
  }
}

TEST_CASE("Database tag operations - Complex scenarios", "[database][tags][complex]")
{
  TestDatabase test_db("tags_complex");

  SECTION("Tag workflow: create tag, find by name, add to file, get file tags")
  {
    // Create folder and file
    auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "WorkflowFolder");
    auto file_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "workflow_file.txt", 100, "text/plain", "storage_workflow");

    // Create tag
    auto tag_insert_result = test_db->insertTag("workflow_tag");
    REQUIRE(tag_insert_result.success());
    auto inserted_tag_id = tag_insert_result.value.value();

    // Find tag by name
    auto tag_get_result = test_db->getTagByName("workflow_tag");
    REQUIRE(tag_get_result.success());
    REQUIRE(tag_get_result.value.value() == inserted_tag_id);

    // Add tag to file
    auto add_tag_result = test_db->addFileTag(file_id.value(), inserted_tag_id);
    REQUIRE(add_tag_result.success());

    // Get file tags
    auto file_tags_result = test_db->getFileTags(file_id.value());
    REQUIRE(file_tags_result.success());
    auto tags = file_tags_result.value.value();
    REQUIRE(tags.size() == 1);
    REQUIRE(tags[0] == "workflow_tag");
  }

  SECTION("Stress test: many tags on many files")
  {
    auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "StressFolder");

    // Create 50 tags
    std::vector<int> tag_ids;
    for (int i = 0; i < 50; ++i)
    {
      std::string tag_name = "stress_tag_" + std::to_string(i);
      auto tag_result = test_db->insertTag(tag_name);
      REQUIRE(tag_result.success());
      tag_ids.push_back(tag_result.value.value());
    }

    // Create 20 files
    std::vector<int> file_ids;
    for (int i = 0; i < 20; ++i)
    {
      std::string file_name = "stress_file_" + std::to_string(i) + ".txt";
      std::string storage_id = "storage_stress_" + std::to_string(i);
      auto file_result = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), file_name, 100 + i, "text/plain", storage_id);
      REQUIRE(file_result.has_value());
      file_ids.push_back(file_result.value());
    }

    // Add random tags to files (each file gets 5-15 tags)
    for (int file_idx = 0; file_idx < 20; ++file_idx)
    {
      int num_tags = 5 + (file_idx % 11); // 5 to 15 tags per file
      for (int tag_idx = 0; tag_idx < num_tags; ++tag_idx)
      {
        int tag_id = tag_ids[tag_idx];
        auto add_result = test_db->addFileTag(file_ids[file_idx], tag_id);
        REQUIRE(add_result.success());
      }
    }

    // Verify each file has the expected number of tags
    for (int file_idx = 0; file_idx < 20; ++file_idx)
    {
      auto tags_result = test_db->getFileTags(file_ids[file_idx]);
      REQUIRE(tags_result.success());

      int expected_num_tags = 5 + (file_idx % 11);
      REQUIRE(tags_result.value.value().size() == expected_num_tags);
    }
  }

  SECTION("Tag reuse across multiple files")
  {
    auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "ReuseFolder");

    // Create common tags
    auto common_tag_result = test_db->insertTag("common");
    auto rare_tag_result = test_db->insertTag("rare");

    REQUIRE(common_tag_result.success());
    REQUIRE(rare_tag_result.success());

    auto common_tag_id = common_tag_result.value.value();
    auto rare_tag_id = rare_tag_result.value.value();

    // Create multiple files and add common tag to all
    std::vector<int> file_ids;
    for (int i = 0; i < 10; ++i)
    {
      std::string file_name = "common_file_" + std::to_string(i) + ".txt";
      std::string storage_id = "storage_common_" + std::to_string(i);
      auto file_result = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), file_name, 100, "text/plain", storage_id);
      REQUIRE(file_result.has_value());
      file_ids.push_back(file_result.value());

      // Add common tag to all files
      auto add_result = test_db->addFileTag(file_result.value(), common_tag_id);
      REQUIRE(add_result.success());
    }

    // Add rare tag to only first file
    auto add_rare_result = test_db->addFileTag(file_ids[0], rare_tag_id);
    REQUIRE(add_rare_result.success());

    // Verify all files have common tag
    for (int file_id : file_ids)
    {
      auto tags_result = test_db->getFileTags(file_id);
      REQUIRE(tags_result.success());
      auto tags = tags_result.value.value();

      // All should have "common" tag
      REQUIRE(std::find(tags.begin(), tags.end(), "common") != tags.end());
    }

    // Verify only first file has rare tag
    auto first_file_tags = test_db->getFileTags(file_ids[0]);
    REQUIRE(first_file_tags.success());
    auto first_tags = first_file_tags.value.value();
    REQUIRE(first_tags.size() == 2);
    REQUIRE(std::find(first_tags.begin(), first_tags.end(), "rare") != first_tags.end());

    // Verify other files don't have rare tag
    for (size_t i = 1; i < file_ids.size(); ++i)
    {
      auto tags_result = test_db->getFileTags(file_ids[i]);
      REQUIRE(tags_result.success());
      auto tags = tags_result.value.value();
      REQUIRE(tags.size() == 1);
      REQUIRE(std::find(tags.begin(), tags.end(), "rare") == tags.end());
    }
  }

  SECTION("Tag operations with file deletion cascade")
  {
    auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "CascadeFolder");
    auto file_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id.value(), "cascade_file.txt", 100, "text/plain", "storage_cascade");

    // Create and add tags to file
    auto tag1_result = test_db->insertTag("cascade_tag1");
    auto tag2_result = test_db->insertTag("cascade_tag2");

    REQUIRE(tag1_result.success());
    REQUIRE(tag2_result.success());

    auto add1_result = test_db->addFileTag(file_id.value(), tag1_result.value.value());
    auto add2_result = test_db->addFileTag(file_id.value(), tag2_result.value.value());

    REQUIRE(add1_result.success());
    REQUIRE(add2_result.success());

    // Verify tags are associated
    auto tags_before_result = test_db->getFileTags(file_id.value());
    REQUIRE(tags_before_result.success());
    REQUIRE(tags_before_result.value.value().size() == 2);

    // Delete the file
    auto delete_result = test_db->deleteFile(file_id.value());
    REQUIRE(delete_result.success());

    // Tags should still exist (tags table not affected)
    auto tag1_get_result = test_db->getTagByName("cascade_tag1");
    auto tag2_get_result = test_db->getTagByName("cascade_tag2");
    REQUIRE(tag1_get_result.success());
    REQUIRE(tag2_get_result.success());

    // But getting tags for the deleted file should return empty (file doesn't exist)
    auto tags_after_result = test_db->getFileTags(file_id.value());
    REQUIRE(tags_after_result.success());
    REQUIRE(tags_after_result.value.value().empty());
  }
}
