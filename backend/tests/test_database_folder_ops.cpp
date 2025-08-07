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

TEST_CASE("Database folder operations - Delete with Cascade", "[database][folders][delete][cascade]")
{
  TestDatabase test_db("folders_delete_cascade");

  SECTION("Delete folder cascades to delete all files and subfolders")
  {
    // Create a complex folder structure with files
    auto root_id = DatabaseTestHelper::createTestFolder(test_db.get(), "RootFolder");

    // Create subfolders
    auto level1_folder1_result = test_db->insertFolder("Level1_Folder1", root_id);
    auto level1_folder2_result = test_db->insertFolder("Level1_Folder2", root_id);
    REQUIRE(level1_folder1_result.success());
    REQUIRE(level1_folder2_result.success());

    auto level1_folder1_id = level1_folder1_result.value.value();
    auto level1_folder2_id = level1_folder2_result.value.value();

    // Create deeper subfolders
    auto level2_folder1_result = test_db->insertFolder("Level2_Folder1", level1_folder1_id);
    auto level2_folder2_result = test_db->insertFolder("Level2_Folder2", level1_folder1_id);
    auto level2_folder3_result = test_db->insertFolder("Level2_Folder3", level1_folder2_id);
    REQUIRE(level2_folder1_result.success());
    REQUIRE(level2_folder2_result.success());
    REQUIRE(level2_folder3_result.success());

    auto level2_folder1_id = level2_folder1_result.value.value();
    auto level2_folder2_id = level2_folder2_result.value.value();
    auto level2_folder3_id = level2_folder3_result.value.value();

    // Add files to various folders
    auto file1 = DatabaseTestHelper::createTestFile(test_db.get(), root_id.value(), "root_file.txt", 100, "text/plain", "storage_root");
    auto file2 = DatabaseTestHelper::createTestFile(test_db.get(), level1_folder1_id, "level1_file1.txt", 200, "text/plain", "storage_l1f1");
    auto file3 = DatabaseTestHelper::createTestFile(test_db.get(), level1_folder2_id, "level1_file2.txt", 300, "text/plain", "storage_l1f2");
    auto file4 = DatabaseTestHelper::createTestFile(test_db.get(), level2_folder1_id, "level2_file1.txt", 400, "text/plain", "storage_l2f1");
    auto file5 = DatabaseTestHelper::createTestFile(test_db.get(), level2_folder2_id, "level2_file2.txt", 500, "text/plain", "storage_l2f2");
    auto file6 = DatabaseTestHelper::createTestFile(test_db.get(), level2_folder3_id, "level2_file3.txt", 600, "text/plain", "storage_l2f3");

    REQUIRE(file1.has_value());
    REQUIRE(file2.has_value());
    REQUIRE(file3.has_value());
    REQUIRE(file4.has_value());
    REQUIRE(file5.has_value());
    REQUIRE(file6.has_value());

    // Verify initial state - all folders and files exist
    REQUIRE(test_db->getFolderById(root_id.value()).has_value());
    REQUIRE(test_db->getFolderById(level1_folder1_id).has_value());
    REQUIRE(test_db->getFolderById(level1_folder2_id).has_value());
    REQUIRE(test_db->getFolderById(level2_folder1_id).has_value());
    REQUIRE(test_db->getFolderById(level2_folder2_id).has_value());
    REQUIRE(test_db->getFolderById(level2_folder3_id).has_value());

    // TODO: uncomment on adding getFileById
    // REQUIRE(test_db->getFileById(file1.value()).has_value());
    // REQUIRE(test_db->getFileById(file2.value()).has_value());
    // REQUIRE(test_db->getFileById(file3.value()).has_value());
    // REQUIRE(test_db->getFileById(file4.value()).has_value());
    // REQUIRE(test_db->getFileById(file5.value()).has_value());
    // REQUIRE(test_db->getFileById(file6.value()).has_value());

    // Count total folders and files before deletion
    auto all_root_folders_before = test_db->getFoldersByParent(std::nullopt);
    auto level1_children_before = test_db->getFoldersByParent(root_id.value());
    auto level2_folder1_children_before = test_db->getFoldersByParent(level1_folder1_id);
    auto level2_folder2_children_before = test_db->getFoldersByParent(level1_folder2_id);

    REQUIRE(all_root_folders_before.size() == 1);        // Only our root folder
    REQUIRE(level1_children_before.size() == 2);         // Level1_Folder1 and Level1_Folder2
    REQUIRE(level2_folder1_children_before.size() == 2); // Level2_Folder1 and Level2_Folder2
    REQUIRE(level2_folder2_children_before.size() == 1); // Level2_Folder3

    // DELETE THE ROOT FOLDER - this should cascade delete everything
    bool deleted = test_db->deleteFolder(root_id.value());
    REQUIRE(deleted);

    // Verify CASCADE DELETE behavior:
    // All subfolders should be deleted (CASCADE)
    REQUIRE_FALSE(test_db->getFolderById(root_id.value()).has_value());
    REQUIRE_FALSE(test_db->getFolderById(level1_folder1_id).has_value());
    REQUIRE_FALSE(test_db->getFolderById(level1_folder2_id).has_value());
    REQUIRE_FALSE(test_db->getFolderById(level2_folder1_id).has_value());
    REQUIRE_FALSE(test_db->getFolderById(level2_folder2_id).has_value());
    REQUIRE_FALSE(test_db->getFolderById(level2_folder3_id).has_value());

    // All files should be deleted (CASCADE from folder deletion)
    // REQUIRE_FALSE(test_db->getFileById(file1.value()).has_value());
    // REQUIRE_FALSE(test_db->getFileById(file2.value()).has_value());
    // REQUIRE_FALSE(test_db->getFileById(file3.value()).has_value());
    // REQUIRE_FALSE(test_db->getFileById(file4.value()).has_value());
    // REQUIRE_FALSE(test_db->getFileById(file5.value()).has_value());
    // REQUIRE_FALSE(test_db->getFileById(file6.value()).has_value());

    // No folders should remain
    auto all_root_folders_after = test_db->getFoldersByParent(std::nullopt);
    REQUIRE(all_root_folders_after.empty());

    // No child folders should remain
    auto level1_children_after = test_db->getFoldersByParent(root_id.value());
    REQUIRE(level1_children_after.empty());
  }

  SECTION("Delete deep nested folder structure")
  {
    // Create a 10-level deep folder structure with files at each level
    std::vector<int> folder_ids;
    std::vector<int> file_ids;

    // Create root folder
    auto root_result = test_db->insertFolder("DeepRoot");
    REQUIRE(root_result.success());
    folder_ids.push_back(root_result.value.value());

    // Add file to root
    auto root_file = DatabaseTestHelper::createTestFile(test_db.get(), root_result.value.value(),
                                                        "root_deep.txt", 100, "text/plain", "storage_deep_root");
    REQUIRE(root_file.has_value());
    file_ids.push_back(root_file.value());

    // Create 9 more levels, each with a folder and file
    std::optional<int> current_parent = root_result.value.value();
    for (int i = 1; i <= 9; ++i)
    {
      std::string folder_name = "DeepLevel" + std::to_string(i);
      auto folder_result = test_db->insertFolder(folder_name, current_parent);
      REQUIRE(folder_result.success());
      folder_ids.push_back(folder_result.value.value());

      std::string file_name = "deep_file_" + std::to_string(i) + ".txt";
      std::string storage_id = "storage_deep_" + std::to_string(i);
      auto file_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_result.value.value(),
                                                        file_name, 100 * i, "text/plain", storage_id);
      REQUIRE(file_id.has_value());
      file_ids.push_back(file_id.value());

      current_parent = folder_result.value.value();
    }

    // Verify all folders and files exist
    for (int folder_id : folder_ids)
    {
      REQUIRE(test_db->getFolderById(folder_id).has_value());
    }

    // Delete the root folder - should cascade delete all 10 folders and 10 files
    bool deleted = test_db->deleteFolder(folder_ids[0]);
    REQUIRE(deleted);

    // Verify all folders are gone
    for (int folder_id : folder_ids)
    {
      REQUIRE_FALSE(test_db->getFolderById(folder_id).has_value());
    }

    // Verify all files are gone (if you implement getFileById)
    // for (int file_id : file_ids)
    // {
    //   REQUIRE_FALSE(test_db->getFileById(file_id).has_value());
    // }

    // No root folders should remain
    auto remaining_folders = test_db->getFoldersByParent(std::nullopt);
    REQUIRE(remaining_folders.empty());
  }

  SECTION("Delete folder with files but no subfolders")
  {
    auto folder_result = test_db->insertFolder("FilesOnlyFolder");
    REQUIRE(folder_result.success());
    auto folder_id = folder_result.value.value();

    // Add multiple files to the folder
    std::vector<int> file_ids;
    for (int i = 0; i < 5; ++i)
    {
      std::string file_name = "file_" + std::to_string(i) + ".txt";
      std::string storage_id = "storage_file_" + std::to_string(i);
      auto file_id = DatabaseTestHelper::createTestFile(test_db.get(), folder_id,
                                                        file_name, 100 * i, "text/plain", storage_id);
      REQUIRE(file_id.has_value());
      file_ids.push_back(file_id.value());
    }

    // Verify folder and files exist
    REQUIRE(test_db->getFolderById(folder_id).has_value());

    // Delete the folder
    bool deleted = test_db->deleteFolder(folder_id);
    REQUIRE(deleted);

    // Verify folder is gone
    REQUIRE_FALSE(test_db->getFolderById(folder_id).has_value());

    // Verify all files are gone (CASCADE DELETE)
    // for (int file_id : file_ids)
    // {
    //   REQUIRE_FALSE(test_db->getFileById(file_id).has_value());
    // }
  }

  SECTION("Delete empty folder")
  {
    auto folder_result = test_db->insertFolder("EmptyFolder");
    REQUIRE(folder_result.success());
    auto folder_id = folder_result.value.value();

    // Verify folder exists
    REQUIRE(test_db->getFolderById(folder_id).has_value());

    // Delete the empty folder
    bool deleted = test_db->deleteFolder(folder_id);
    REQUIRE(deleted);

    // Verify folder is gone
    REQUIRE_FALSE(test_db->getFolderById(folder_id).has_value());
  }

  SECTION("Partial cascade - delete subfolder, parent remains")
  {
    auto parent_result = test_db->insertFolder("ParentFolder");
    auto child_result = test_db->insertFolder("ChildFolder", parent_result.value);
    REQUIRE(parent_result.success());
    REQUIRE(child_result.success());

    auto parent_id = parent_result.value.value();
    auto child_id = child_result.value.value();

    // Add files to both folders
    auto parent_file = DatabaseTestHelper::createTestFile(test_db.get(), parent_id,
                                                          "parent.txt", 100, "text/plain", "storage_parent");
    auto child_file = DatabaseTestHelper::createTestFile(test_db.get(), child_id,
                                                         "child.txt", 200, "text/plain", "storage_child");
    REQUIRE(parent_file.has_value());
    REQUIRE(child_file.has_value());

    // Delete only the child folder
    bool deleted = test_db->deleteFolder(child_id);
    REQUIRE(deleted);

    // Parent should still exist
    REQUIRE(test_db->getFolderById(parent_id).has_value());
    // Child should be gone
    REQUIRE_FALSE(test_db->getFolderById(child_id).has_value());

    // Parent file should still exist, child file should be gone
    // REQUIRE(test_db->getFileById(parent_file.value()).has_value());
    // REQUIRE_FALSE(test_db->getFileById(child_file.value()).has_value());
  }
}

// ... rest of existing tests ...