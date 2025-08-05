#include <catch2/catch_test_macros.hpp>
#include "test_helpers_database.hpp"
#include <set>

using namespace bytebucket;
using namespace bytebucket::test;
using TestDatabase = DatabaseTestHelper::TestDatabase;

TEST_CASE("Database folder operations - Insert", "[database][folders][insert]")
{
  TestDatabase test_db("folders_insert");

  SECTION("Insert root folder successfully")
  {
    auto result = test_db->insertFolder("Documents");

    REQUIRE(result.success());
    REQUIRE(result.value.has_value());
    REQUIRE(result.value.value() > 0);
  }

  SECTION("Insert folder with parent")
  {
    auto parent_id = DatabaseTestHelper::createTestFolder(test_db.get(), "Documents");
    auto result = test_db->insertFolder("Images", parent_id);

    REQUIRE(result.success());
    REQUIRE(result.value.value() > 0);
    REQUIRE(result.value.value() != parent_id.value());
  }

  SECTION("Insert multiple root folders")
  {
    auto folder_ids = DatabaseTestHelper::createTestFolders(test_db.get(),
                                                            {"Documents", "Downloads", "Pictures"});

    REQUIRE(folder_ids.size() == 3);

    // All IDs should be different
    REQUIRE(folder_ids[0] != folder_ids[1]);
    REQUIRE(folder_ids[1] != folder_ids[2]);
    REQUIRE(folder_ids[0] != folder_ids[2]);
  }

  SECTION("Insert folder with special characters in name")
  {
    auto result = test_db->insertFolder("Folder with spaces & symbols!");

    REQUIRE(result.success());
    REQUIRE(result.value.value() > 0);
  }

  SECTION("Insert folder with Unicode name")
  {
    auto result = test_db->insertFolder("папка");

    REQUIRE(result.success());
    REQUIRE(result.value.value() > 0);
  }

  SECTION("Insert folder with empty name")
  {
    auto result = test_db->insertFolder("");

    REQUIRE(result.success()); // Empty name should be allowed
  }

  SECTION("Insert folder with very long name")
  {
    std::string long_name(1000, 'a'); // 1000 character name

    auto result = test_db->insertFolder(long_name);

    REQUIRE(result.success());
  }

  SECTION("Insert nested folder hierarchy")
  {
    auto root_id = DatabaseTestHelper::createTestFolder(test_db.get(), "Root");
    auto level1_result = test_db->insertFolder("Level1", root_id);
    REQUIRE(level1_result.success());

    auto level2_result = test_db->insertFolder("Level2", level1_result.value);
    REQUIRE(level2_result.success());

    auto level3_result = test_db->insertFolder("Level3", level2_result.value);
    REQUIRE(level3_result.success());

    // All should be different
    REQUIRE(root_id.value() != level1_result.value.value());
    REQUIRE(level1_result.value.value() != level2_result.value.value());
    REQUIRE(level2_result.value.value() != level3_result.value.value());
  }

  SECTION("Same folder name under different parents should succeed")
  {
    auto parent_ids = DatabaseTestHelper::createTestFolders(test_db.get(), {"Parent1", "Parent2"});

    auto child1_result = test_db->insertFolder("SameName", parent_ids[0]);
    auto child2_result = test_db->insertFolder("SameName", parent_ids[1]);

    REQUIRE(child1_result.success());
    REQUIRE(child2_result.success());
    REQUIRE(child1_result.value.value() != child2_result.value.value());
  }

  SECTION("Duplicate folder name under same parent should fail")
  {
    auto parent_id = DatabaseTestHelper::createTestFolder(test_db.get(), "Parent");

    auto child1_result = test_db->insertFolder("DuplicateName", parent_id);
    REQUIRE(child1_result.success());

    auto child2_result = test_db->insertFolder("DuplicateName", parent_id);
    REQUIRE_FALSE(child2_result.success());
    REQUIRE(child2_result.error == DatabaseError::UniqueConstraint);
  }

  SECTION("Returned folder ID is sequential")
  {
    auto folder1_result = test_db->insertFolder("Seq1");
    auto folder2_result = test_db->insertFolder("Seq2");
    auto folder3_result = test_db->insertFolder("Seq3");

    REQUIRE(folder1_result.success());
    REQUIRE(folder2_result.success());
    REQUIRE(folder3_result.success());

    // IDs should be sequential (assuming no other operations)
    REQUIRE(folder2_result.value.value() > folder1_result.value.value());
    REQUIRE(folder3_result.value.value() > folder2_result.value.value());
  }
}

TEST_CASE("Database folder operations edge cases", "[database][folders][edge]")
{
  TestDatabase test_db("folders_edge");

  SECTION("Insert folder with non-existent parent should fail")
  {
    auto result = test_db->insertFolder("Orphan", 99999);

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::ForeignKeyConstraint);
    REQUIRE(result.errorMessage == "Parent folder doesn't exist");
  }

  SECTION("Insert folder with negative parent ID should fail")
  {
    auto result = test_db->insertFolder("NegativeParent", -1);

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::ForeignKeyConstraint);
  }

  SECTION("Insert folder with zero parent ID should fail")
  {
    auto result = test_db->insertFolder("ZeroParent", 0);

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::ForeignKeyConstraint);
  }

  SECTION("Insert many folders quickly")
  {
    for (int i = 0; i < 100; ++i)
    {
      std::string folder_name = "bulk_folder_" + std::to_string(i);
      auto result = test_db->insertFolder(folder_name);

      REQUIRE(result.success());
    }

    // Verify all can be retrieved
    auto root_folders = test_db->getFoldersByParent(std::nullopt);
    REQUIRE(root_folders.size() == 100);
  }

  SECTION("Insert deeply nested folder structure")
  {
    std::optional<int> current_parent;

    // Create 50 levels deep
    for (int i = 0; i < 50; ++i)
    {
      std::string folder_name = "level_" + std::to_string(i);
      auto result = test_db->insertFolder(folder_name, current_parent);

      REQUIRE(result.success());
      current_parent = result.value;
    }

    // Should have exactly 1 root folder
    auto root_folders = test_db->getFoldersByParent(std::nullopt);
    REQUIRE(root_folders.size() == 1);
    REQUIRE(root_folders[0].name == "level_0");
  }

  SECTION("Stress test - create and delete many folders")
  {
    std::vector<int> folder_ids;

    // Create 1000 folders
    for (int i = 0; i < 1000; ++i)
    {
      auto result = test_db->insertFolder("stress_" + std::to_string(i));
      REQUIRE(result.success());
      folder_ids.push_back(result.value.value());
    }

    // Verify all exist
    auto root_folders = test_db->getFoldersByParent(std::nullopt);
    REQUIRE(root_folders.size() == 1000);

    // Delete every other folder
    for (size_t i = 0; i < folder_ids.size(); i += 2)
    {
      bool deleted = test_db->deleteFolder(folder_ids[i]);
      REQUIRE(deleted);
    }

    // Should have 500 folders remaining
    auto remaining_folders = test_db->getFoldersByParent(std::nullopt);
    REQUIRE(remaining_folders.size() == 500);
  }

  SECTION("Test specific error types")
  {
    SECTION("Foreign key constraint error")
    {
      auto result = test_db->insertFolder("Orphan", 99999);

      REQUIRE_FALSE(result.success());
      REQUIRE(result.error == DatabaseError::ForeignKeyConstraint);
      REQUIRE(result.errorMessage == "Parent folder doesn't exist");
    }

    SECTION("Unique constraint error")
    {
      auto parent_id = DatabaseTestHelper::createTestFolder(test_db.get(), "Parent");

      // First folder should succeed
      auto result1 = test_db->insertFolder("DuplicateName", parent_id);
      REQUIRE(result1.success());

      // Second folder with same name and parent should fail
      auto result2 = test_db->insertFolder("DuplicateName", parent_id);
      REQUIRE_FALSE(result2.success());
      REQUIRE(result2.error == DatabaseError::UniqueConstraint);
      REQUIRE(result2.errorMessage == "A folder with this name already exists in the parent directory");
    }
  }
}