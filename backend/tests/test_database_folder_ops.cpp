#include <catch2/catch_test_macros.hpp>
#include "test_helpers_database.hpp"

using namespace bytebucket;
using namespace bytebucket::test;
using TestDatabase = DatabaseTestHelper::TestDatabase;

TEST_CASE("Database folder operations", "[database][folders]")
{
  TestDatabase test_db("folders");

  SECTION("Insert root folder successfully")
  {
    auto folder_id = test_db->insertFolder("Documents");

    REQUIRE(folder_id.has_value());
    REQUIRE(folder_id.value() > 0);
  }

  SECTION("Insert folder with parent")
  {
    auto parent_id = DatabaseTestHelper::createTestFolder(test_db.get(), "Documents");
    auto child_id = test_db->insertFolder("Images", parent_id);

    REQUIRE(child_id.has_value());
    REQUIRE(child_id.value() > 0);
    REQUIRE(child_id.value() != parent_id.value());
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
    auto folder_id = test_db->insertFolder("Folder with spaces & symbols!");

    REQUIRE(folder_id.has_value());
    REQUIRE(folder_id.value() > 0);
  }

  SECTION("Insert folder with Unicode name")
  {
    auto folder_id = test_db->insertFolder("папка");

    REQUIRE(folder_id.has_value());
    REQUIRE(folder_id.value() > 0);
  }

  SECTION("Insert folder with empty name")
  {
    auto folder_id = test_db->insertFolder("");

    REQUIRE(folder_id.has_value()); // Empty name should be allowed
  }

  SECTION("Insert folder with very long name")
  {
    std::string long_name(1000, 'a'); // 1000 character name

    auto folder_id = test_db->insertFolder(long_name);

    REQUIRE(folder_id.has_value());
  }

  SECTION("Insert nested folder hierarchy")
  {
    auto root_id = DatabaseTestHelper::createTestFolder(test_db.get(), "Root");
    auto level1_id = test_db->insertFolder("Level1", root_id);
    REQUIRE(level1_id.has_value());

    auto level2_id = test_db->insertFolder("Level2", level1_id);
    REQUIRE(level2_id.has_value());

    auto level3_id = test_db->insertFolder("Level3", level2_id);
    REQUIRE(level3_id.has_value());

    // All should be different
    REQUIRE(root_id.value() != level1_id.value());
    REQUIRE(level1_id.value() != level2_id.value());
    REQUIRE(level2_id.value() != level3_id.value());
  }

  SECTION("Insert multiple children under same parent")
  {
    auto parent_id = DatabaseTestHelper::createTestFolder(test_db.get(), "Parent");

    auto child_ids = {
        test_db->insertFolder("Child1", parent_id),
        test_db->insertFolder("Child2", parent_id),
        test_db->insertFolder("Child3", parent_id)};

    for (const auto &child_id : child_ids)
    {
      REQUIRE(child_id.has_value());
    }

    // All children should be different
    auto ids = std::vector<int>();
    for (const auto &child_id : child_ids)
    {
      ids.push_back(child_id.value());
    }

    REQUIRE(ids[0] != ids[1]);
    REQUIRE(ids[1] != ids[2]);
    REQUIRE(ids[0] != ids[2]);
  }

  SECTION("Same folder name under different parents should succeed")
  {
    auto parent_ids = DatabaseTestHelper::createTestFolders(test_db.get(), {"Parent1", "Parent2"});

    auto child1_id = test_db->insertFolder("SameName", parent_ids[0]);
    auto child2_id = test_db->insertFolder("SameName", parent_ids[1]);

    REQUIRE(child1_id.has_value());
    REQUIRE(child2_id.has_value());
    REQUIRE(child1_id.value() != child2_id.value());
  }

  SECTION("Same folder name under same parent should succeed")
  {
    auto parent_id = DatabaseTestHelper::createTestFolder(test_db.get(), "Parent");

    auto child1_id = test_db->insertFolder("DuplicateName", parent_id);
    auto child2_id = test_db->insertFolder("DuplicateName", parent_id);

    REQUIRE(child1_id.has_value());
    REQUIRE(child2_id.has_value());
    REQUIRE(child1_id.value() != child2_id.value()); // SQLite allows duplicate names
  }

  SECTION("Returned folder ID is sequential")
  {
    auto folder1_id = test_db->insertFolder("Seq1");
    auto folder2_id = test_db->insertFolder("Seq2");
    auto folder3_id = test_db->insertFolder("Seq3");

    REQUIRE(folder1_id.has_value());
    REQUIRE(folder2_id.has_value());
    REQUIRE(folder3_id.has_value());

    // IDs should be sequential (assuming no other operations)
    REQUIRE(folder2_id.value() > folder1_id.value());
    REQUIRE(folder3_id.value() > folder2_id.value());
  }
}

TEST_CASE("Database folder operations edge cases", "[database][folders][edge]")
{
  TestDatabase test_db("folders_edge");

  SECTION("Insert folder with non-existent parent should fail")
  {
    auto folder_id = test_db->insertFolder("Orphan", 99999);

    REQUIRE_FALSE(folder_id.has_value());
  }

  SECTION("Insert folder with negative parent ID should fail")
  {
    auto folder_id = test_db->insertFolder("NegativeParent", -1);

    REQUIRE_FALSE(folder_id.has_value());
  }

  SECTION("Insert folder with zero parent ID should fail")
  {
    auto folder_id = test_db->insertFolder("ZeroParent", 0);

    REQUIRE_FALSE(folder_id.has_value());
  }

  SECTION("Insert many folders quickly")
  {
    for (int i = 0; i < 100; ++i)
    {
      std::string folder_name = "bulk_folder_" + std::to_string(i);
      auto folder_id = test_db->insertFolder(folder_name);

      REQUIRE(folder_id.has_value());
    }
  }

  SECTION("Insert deeply nested folder structure")
  {
    std::optional<int> current_parent;

    // Create 50 levels deep
    for (int i = 0; i < 50; ++i)
    {
      std::string folder_name = "level_" + std::to_string(i);
      auto folder_id = test_db->insertFolder(folder_name, current_parent);

      REQUIRE(folder_id.has_value());
      current_parent = folder_id;
    }
  }

  SECTION("Insert folder with parent that exists")
  {
    auto parent_id = DatabaseTestHelper::createTestFolder(test_db.get(), "TempParent");

    // This should succeed
    auto child_id = test_db->insertFolder("Child", parent_id);
    REQUIRE(child_id.has_value());
    REQUIRE(child_id.value() != parent_id.value());
  }
}