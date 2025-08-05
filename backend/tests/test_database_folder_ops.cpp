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

TEST_CASE("Database folder operations - Get by ID", "[database][folders][get]")
{
  TestDatabase test_db("folders_get");

  SECTION("Get existing folder by ID")
  {
    auto folder_id = test_db->insertFolder("TestFolder");
    REQUIRE(folder_id.has_value());

    auto folder = test_db->getFolderById(folder_id.value());

    REQUIRE(folder.has_value());
    REQUIRE(folder->id == folder_id.value());
    REQUIRE(folder->name == "TestFolder");
    REQUIRE_FALSE(folder->parentId.has_value());
  }

  SECTION("Get folder with parent by ID")
  {
    auto parent_id = test_db->insertFolder("ParentFolder");
    auto child_id = test_db->insertFolder("ChildFolder", parent_id);
    REQUIRE(child_id.has_value());

    auto folder = test_db->getFolderById(child_id.value());

    REQUIRE(folder.has_value());
    REQUIRE(folder->id == child_id.value());
    REQUIRE(folder->name == "ChildFolder");
    REQUIRE(folder->parentId.has_value());
    REQUIRE(folder->parentId.value() == parent_id.value());
  }

  SECTION("Get non-existent folder by ID")
  {
    auto folder = test_db->getFolderById(99999);

    REQUIRE_FALSE(folder.has_value());
  }

  SECTION("Get folder with special characters")
  {
    auto folder_id = test_db->insertFolder("Folder & Symbols!");
    REQUIRE(folder_id.has_value());

    auto folder = test_db->getFolderById(folder_id.value());

    REQUIRE(folder.has_value());
    REQUIRE(folder->name == "Folder & Symbols!");
  }

  SECTION("Get folder with Unicode name")
  {
    auto folder_id = test_db->insertFolder("папка");
    REQUIRE(folder_id.has_value());

    auto folder = test_db->getFolderById(folder_id.value());

    REQUIRE(folder.has_value());
    REQUIRE(folder->name == "папка");
  }

  SECTION("Get folder with zero ID")
  {
    auto folder = test_db->getFolderById(0);

    REQUIRE_FALSE(folder.has_value());
  }

  SECTION("Get folder with negative ID")
  {
    auto folder = test_db->getFolderById(-1);

    REQUIRE_FALSE(folder.has_value());
  }
}

TEST_CASE("Database folder operations - Get by Parent", "[database][folders][getparent]")
{
  TestDatabase test_db("folders_getparent");

  SECTION("Get root folders (no parent)")
  {
    // Create some root folders
    auto folder_ids = DatabaseTestHelper::createTestFolders(test_db.get(),
                                                            {"Documents", "Downloads", "Pictures"});

    auto root_folders = test_db->getFoldersByParent(std::nullopt);

    REQUIRE(root_folders.size() == 3);

    // Check names are present and sorted
    std::vector<std::string> names;
    for (const auto &folder : root_folders)
    {
      names.push_back(folder.name);
      REQUIRE_FALSE(folder.parentId.has_value());
    }

    // Should be sorted by name
    REQUIRE(std::is_sorted(names.begin(), names.end()));
  }

  SECTION("Get child folders of specific parent")
  {
    auto parent_id = test_db->insertFolder("ParentFolder");
    REQUIRE(parent_id.has_value());

    // Create child folders
    auto child_ids = {
        test_db->insertFolder("Child1", parent_id),
        test_db->insertFolder("Child2", parent_id),
        test_db->insertFolder("Child3", parent_id)};

    auto child_folders = test_db->getFoldersByParent(parent_id.value());

    REQUIRE(child_folders.size() == 3);

    for (const auto &folder : child_folders)
    {
      REQUIRE(folder.parentId.has_value());
      REQUIRE(folder.parentId.value() == parent_id.value());
    }

    // Check names are sorted
    std::vector<std::string> names;
    for (const auto &folder : child_folders)
    {
      names.push_back(folder.name);
    }
    REQUIRE(std::is_sorted(names.begin(), names.end()));
  }

  SECTION("Get folders from non-existent parent")
  {
    auto folders = test_db->getFoldersByParent(99999);

    REQUIRE(folders.empty());
  }

  SECTION("Get folders from parent with no children")
  {
    auto parent_id = test_db->insertFolder("EmptyParent");
    REQUIRE(parent_id.has_value());

    auto folders = test_db->getFoldersByParent(parent_id.value());

    REQUIRE(folders.empty());
  }

  SECTION("Mixed root and child folders")
  {
    // Create root folders
    auto root1_id = test_db->insertFolder("RootA");
    auto root2_id = test_db->insertFolder("RootB");

    // Create child folders under root1
    test_db->insertFolder("Child1", root1_id);
    test_db->insertFolder("Child2", root1_id);

    // Create child folders under root2
    test_db->insertFolder("Child3", root2_id);

    // Get root folders
    auto root_folders = test_db->getFoldersByParent(std::nullopt);
    REQUIRE(root_folders.size() == 2);

    // Get children of root1
    auto root1_children = test_db->getFoldersByParent(root1_id.value());
    REQUIRE(root1_children.size() == 2);

    // Get children of root2
    auto root2_children = test_db->getFoldersByParent(root2_id.value());
    REQUIRE(root2_children.size() == 1);
  }

  SECTION("Deep hierarchy navigation")
  {
    auto level0_id = test_db->insertFolder("Level0");
    auto level1_id = test_db->insertFolder("Level1", level0_id);
    auto level2_id = test_db->insertFolder("Level2", level1_id);
    auto level3_id = test_db->insertFolder("Level3", level2_id);

    // Each level should have exactly one child
    auto level0_children = test_db->getFoldersByParent(level0_id.value());
    REQUIRE(level0_children.size() == 1);
    REQUIRE(level0_children[0].name == "Level1");

    auto level1_children = test_db->getFoldersByParent(level1_id.value());
    REQUIRE(level1_children.size() == 1);
    REQUIRE(level1_children[0].name == "Level2");

    auto level2_children = test_db->getFoldersByParent(level2_id.value());
    REQUIRE(level2_children.size() == 1);
    REQUIRE(level2_children[0].name == "Level3");

    auto level3_children = test_db->getFoldersByParent(level3_id.value());
    REQUIRE(level3_children.empty());
  }
}

TEST_CASE("Database folder operations - Delete", "[database][folders][delete]")
{
  TestDatabase test_db("folders_delete");

  SECTION("Delete empty root folder")
  {
    auto folder_id = test_db->insertFolder("ToDelete");
    REQUIRE(folder_id.has_value());

    // Verify folder exists
    auto folder = test_db->getFolderById(folder_id.value());
    REQUIRE(folder.has_value());

    // Delete folder
    bool deleted = test_db->deleteFolder(folder_id.value());
    REQUIRE(deleted);

    // Verify folder no longer exists
    auto deleted_folder = test_db->getFolderById(folder_id.value());
    REQUIRE_FALSE(deleted_folder.has_value());
  }

  SECTION("Delete folder with child folders")
  {
    auto parent_id = test_db->insertFolder("ParentToDelete");
    auto child1_id = test_db->insertFolder("Child1", parent_id);
    auto child2_id = test_db->insertFolder("Child2", parent_id);

    // Delete parent - child folders should have parent_id set to NULL (ON DELETE SET NULL)
    bool deleted = test_db->deleteFolder(parent_id.value());
    REQUIRE(deleted);

    // Parent should be gone
    auto parent_folder = test_db->getFolderById(parent_id.value());
    REQUIRE_FALSE(parent_folder.has_value());

    // Children should still exist but with NULL parent_id
    auto child1 = test_db->getFolderById(child1_id.value());
    auto child2 = test_db->getFolderById(child2_id.value());

    REQUIRE(child1.has_value());
    REQUIRE(child2.has_value());
    REQUIRE_FALSE(child1->parentId.has_value());
    REQUIRE_FALSE(child2->parentId.has_value());

    // Children should now appear as root folders
    auto root_folders = test_db->getFoldersByParent(std::nullopt);
    bool found_child1 = false, found_child2 = false;
    for (const auto &folder : root_folders)
    {
      if (folder.name == "Child1")
        found_child1 = true;
      if (folder.name == "Child2")
        found_child2 = true;
    }
    REQUIRE(found_child1);
    REQUIRE(found_child2);
  }

  SECTION("Delete non-existent folder")
  {
    bool deleted = test_db->deleteFolder(99999);
    REQUIRE_FALSE(deleted);
  }

  SECTION("Delete folder with zero ID")
  {
    bool deleted = test_db->deleteFolder(0);
    REQUIRE_FALSE(deleted);
  }

  SECTION("Delete folder with negative ID")
  {
    bool deleted = test_db->deleteFolder(-1);
    REQUIRE_FALSE(deleted);
  }

  SECTION("Delete multiple folders in sequence")
  {
    auto folder_ids = DatabaseTestHelper::createTestFolders(test_db.get(),
                                                            {"Folder1", "Folder2", "Folder3"});

    // Delete all folders
    for (int folder_id : folder_ids)
    {
      bool deleted = test_db->deleteFolder(folder_id);
      REQUIRE(deleted);
    }

    // Verify all are gone
    for (int folder_id : folder_ids)
    {
      auto folder = test_db->getFolderById(folder_id);
      REQUIRE_FALSE(folder.has_value());
    }

    // No root folders should remain
    auto root_folders = test_db->getFoldersByParent(std::nullopt);
    REQUIRE(root_folders.empty());
  }

  SECTION("Delete folder twice should fail second time")
  {
    auto folder_id = test_db->insertFolder("DeleteTwice");
    REQUIRE(folder_id.has_value());

    // First delete should succeed
    bool deleted1 = test_db->deleteFolder(folder_id.value());
    REQUIRE(deleted1);

    // Second delete should fail
    bool deleted2 = test_db->deleteFolder(folder_id.value());
    REQUIRE_FALSE(deleted2);
  }

  SECTION("Delete parent and verify children become root folders")
  {
    auto parent_id = test_db->insertFolder("Parent");
    auto child_ids = {
        test_db->insertFolder("ChildA", parent_id),
        test_db->insertFolder("ChildB", parent_id),
        test_db->insertFolder("ChildC", parent_id)};

    // Verify children exist under parent
    auto children_before = test_db->getFoldersByParent(parent_id.value());
    REQUIRE(children_before.size() == 3);

    // Delete parent
    bool deleted = test_db->deleteFolder(parent_id.value());
    REQUIRE(deleted);

    // Children should become root folders
    auto root_folders_after = test_db->getFoldersByParent(std::nullopt);
    REQUIRE(root_folders_after.size() == 3);

    // Verify all children are now root folders
    std::set<std::string> child_names = {"ChildA", "ChildB", "ChildC"};
    for (const auto &folder : root_folders_after)
    {
      REQUIRE(child_names.count(folder.name) == 1);
      REQUIRE_FALSE(folder.parentId.has_value());
    }
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
      auto folder_id = test_db->insertFolder(folder_name, current_parent);

      REQUIRE(folder_id.has_value());
      current_parent = folder_id;
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
      auto folder_id = test_db->insertFolder("stress_" + std::to_string(i));
      REQUIRE(folder_id.has_value());
      folder_ids.push_back(folder_id.value());
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
}