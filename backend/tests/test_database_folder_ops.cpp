#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include "database.hpp"

TEST_CASE("Database folder operations", "[database][folders]")
{
  using namespace bytebucket;

  std::string test_db_path = "test_db_folders.db";

  // Clean up any existing test database
  std::filesystem::remove(test_db_path);
  std::filesystem::remove(test_db_path + "-wal");
  std::filesystem::remove(test_db_path + "-shm");

  auto db = Database::create(test_db_path);
  REQUIRE(db != nullptr);

  SECTION("Insert root folder successfully")
  {
    auto folder_id = db->insertFolder("Documents");

    REQUIRE(folder_id.has_value());
    REQUIRE(folder_id.value() > 0);
  }

  SECTION("Insert folder with parent")
  {
    auto parent_id = db->insertFolder("Documents");
    REQUIRE(parent_id.has_value());

    auto child_id = db->insertFolder("Images", parent_id);

    REQUIRE(child_id.has_value());
    REQUIRE(child_id.value() > 0);
    REQUIRE(child_id.value() != parent_id.value());
  }

  SECTION("Insert multiple root folders")
  {
    auto folder1_id = db->insertFolder("Documents");
    auto folder2_id = db->insertFolder("Downloads");
    auto folder3_id = db->insertFolder("Pictures");

    REQUIRE(folder1_id.has_value());
    REQUIRE(folder2_id.has_value());
    REQUIRE(folder3_id.has_value());

    // All IDs should be different
    REQUIRE(folder1_id.value() != folder2_id.value());
    REQUIRE(folder2_id.value() != folder3_id.value());
    REQUIRE(folder1_id.value() != folder3_id.value());
  }

  SECTION("Insert folder with special characters in name")
  {
    auto folder_id = db->insertFolder("Folder with spaces & symbols!");

    REQUIRE(folder_id.has_value());
    REQUIRE(folder_id.value() > 0);
  }

  SECTION("Insert folder with Unicode name")
  {
    auto folder_id = db->insertFolder("папка");

    REQUIRE(folder_id.has_value());
    REQUIRE(folder_id.value() > 0);
  }

  SECTION("Insert folder with empty name")
  {
    auto folder_id = db->insertFolder("");

    REQUIRE(folder_id.has_value()); // Empty name should be allowed
  }

  SECTION("Insert folder with very long name")
  {
    std::string long_name(1000, 'a'); // 1000 character name

    auto folder_id = db->insertFolder(long_name);

    REQUIRE(folder_id.has_value());
  }

  SECTION("Insert nested folder hierarchy")
  {
    auto root_id = db->insertFolder("Root");
    REQUIRE(root_id.has_value());

    auto level1_id = db->insertFolder("Level1", root_id);
    REQUIRE(level1_id.has_value());

    auto level2_id = db->insertFolder("Level2", level1_id);
    REQUIRE(level2_id.has_value());

    auto level3_id = db->insertFolder("Level3", level2_id);
    REQUIRE(level3_id.has_value());

    // All should be different
    REQUIRE(root_id.value() != level1_id.value());
    REQUIRE(level1_id.value() != level2_id.value());
    REQUIRE(level2_id.value() != level3_id.value());
  }

  SECTION("Insert multiple children under same parent")
  {
    auto parent_id = db->insertFolder("Parent");
    REQUIRE(parent_id.has_value());

    auto child1_id = db->insertFolder("Child1", parent_id);
    auto child2_id = db->insertFolder("Child2", parent_id);
    auto child3_id = db->insertFolder("Child3", parent_id);

    REQUIRE(child1_id.has_value());
    REQUIRE(child2_id.has_value());
    REQUIRE(child3_id.has_value());

    // All children should be different
    REQUIRE(child1_id.value() != child2_id.value());
    REQUIRE(child2_id.value() != child3_id.value());
    REQUIRE(child1_id.value() != child3_id.value());
  }

  SECTION("Same folder name under different parents should succeed")
  {
    auto parent1_id = db->insertFolder("Parent1");
    auto parent2_id = db->insertFolder("Parent2");

    auto child1_id = db->insertFolder("SameName", parent1_id);
    auto child2_id = db->insertFolder("SameName", parent2_id);

    REQUIRE(child1_id.has_value());
    REQUIRE(child2_id.has_value());
    REQUIRE(child1_id.value() != child2_id.value());
  }

  SECTION("Same folder name under same parent should succeed")
  {
    auto parent_id = db->insertFolder("Parent");

    auto child1_id = db->insertFolder("DuplicateName", parent_id);
    auto child2_id = db->insertFolder("DuplicateName", parent_id);

    REQUIRE(child1_id.has_value());
    REQUIRE(child2_id.has_value());
    REQUIRE(child1_id.value() != child2_id.value()); // SQLite allows duplicate names
  }

  SECTION("Returned folder ID is sequential")
  {
    auto folder1_id = db->insertFolder("Seq1");
    auto folder2_id = db->insertFolder("Seq2");
    auto folder3_id = db->insertFolder("Seq3");

    REQUIRE(folder1_id.has_value());
    REQUIRE(folder2_id.has_value());
    REQUIRE(folder3_id.has_value());

    // IDs should be sequential (assuming no other operations)
    REQUIRE(folder2_id.value() > folder1_id.value());
    REQUIRE(folder3_id.value() > folder2_id.value());
  }

  // Cleanup
  db.reset();
  std::filesystem::remove(test_db_path);
  std::filesystem::remove(test_db_path + "-wal");
  std::filesystem::remove(test_db_path + "-shm");
}

TEST_CASE("Database folder operations edge cases", "[database][folders][edge]")
{
  using namespace bytebucket;

  std::string test_db_path = "test_db_folders_edge.db";

  // Clean up any existing test database
  std::filesystem::remove(test_db_path);
  std::filesystem::remove(test_db_path + "-wal");
  std::filesystem::remove(test_db_path + "-shm");

  auto db = Database::create(test_db_path);
  REQUIRE(db != nullptr);

  SECTION("Insert folder with non-existent parent should fail")
  {
    auto folder_id = db->insertFolder("Orphan", 99999);

    REQUIRE_FALSE(folder_id.has_value());
  }

  SECTION("Insert folder with negative parent ID should fail")
  {
    auto folder_id = db->insertFolder("NegativeParent", -1);

    REQUIRE_FALSE(folder_id.has_value());
  }

  SECTION("Insert folder with zero parent ID should fail")
  {
    auto folder_id = db->insertFolder("ZeroParent", 0);

    REQUIRE_FALSE(folder_id.has_value());
  }

  SECTION("Insert many folders quickly")
  {
    std::vector<std::optional<int>> folder_ids;

    for (int i = 0; i < 100; ++i)
    {
      std::string folder_name = "bulk_folder_" + std::to_string(i);

      auto folder_id = db->insertFolder(folder_name);
      folder_ids.push_back(folder_id);

      REQUIRE(folder_id.has_value());
    }

    // All IDs should be unique
    for (size_t i = 0; i < folder_ids.size(); ++i)
    {
      for (size_t j = i + 1; j < folder_ids.size(); ++j)
      {
        REQUIRE(folder_ids[i].value() != folder_ids[j].value());
      }
    }
  }

  SECTION("Insert deeply nested folder structure")
  {
    std::optional<int> current_parent;

    // Create 50 levels deep
    for (int i = 0; i < 50; ++i)
    {
      std::string folder_name = "level_" + std::to_string(i);
      auto folder_id = db->insertFolder(folder_name, current_parent);

      REQUIRE(folder_id.has_value());
      current_parent = folder_id;
    }
  }

  SECTION("Insert folder with parent that exists")
  {
    auto parent_id = db->insertFolder("TempParent");
    REQUIRE(parent_id.has_value());

    // This should succeed
    auto child_id = db->insertFolder("Child", parent_id);
    REQUIRE(child_id.has_value());
    REQUIRE(child_id.value() != parent_id.value());
  }

  // Cleanup
  db.reset();
  std::filesystem::remove(test_db_path);
  std::filesystem::remove(test_db_path + "-wal");
  std::filesystem::remove(test_db_path + "-shm");
}