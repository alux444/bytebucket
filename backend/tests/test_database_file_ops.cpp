#include <catch2/catch_test_macros.hpp>
#include "test_helpers_database.hpp"
#include <thread>

using namespace bytebucket;
using namespace bytebucket::test;
using TestDatabase = DatabaseTestHelper::TestDatabase;

TEST_CASE("Database file operations", "[database][files]")
{
  TestDatabase test_db("files");
  auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get());

  SECTION("Add file successfully")
  {
    auto result = test_db->addFile("test.txt", folder_id.value(), 1024, "text/plain", "storage123");

    REQUIRE(result.success());
    REQUIRE(result.value.has_value());
    REQUIRE(result.value.value() > 0);
  }

  SECTION("Add file with different content types")
  {
    auto txt_result = test_db->addFile("document.txt", folder_id.value(), 2048, "text/plain", "storage_txt");
    auto pdf_result = test_db->addFile("document.pdf", folder_id.value(), 4096, "application/pdf", "storage_pdf");
    auto img_result = test_db->addFile("image.jpg", folder_id.value(), 8192, "image/jpeg", "storage_img");

    REQUIRE(txt_result.success());
    REQUIRE(pdf_result.success());
    REQUIRE(img_result.success());

    // All IDs should be different
    REQUIRE(txt_result.value.value() != pdf_result.value.value());
    REQUIRE(pdf_result.value.value() != img_result.value.value());
    REQUIRE(txt_result.value.value() != img_result.value.value());
  }

  SECTION("Add file with zero size")
  {
    auto result = test_db->addFile("empty.txt", folder_id.value(), 0, "text/plain", "storage_empty");

    REQUIRE(result.success());
    REQUIRE(result.value.value() > 0);
  }

  SECTION("Add file with large size")
  {
    auto result = test_db->addFile("large.bin", folder_id.value(), 1073741824, "application/octet-stream", "storage_large"); // 1GB

    REQUIRE(result.success());
    REQUIRE(result.value.value() > 0);
  }

  SECTION("Add file with special characters in name")
  {
    auto result = test_db->addFile("test file with spaces & symbols!.txt", folder_id.value(), 512, "text/plain", "storage_special");

    REQUIRE(result.success());
    REQUIRE(result.value.value() > 0);
  }

  SECTION("Add file with Unicode filename")
  {
    auto result = test_db->addFile("тест.txt", folder_id.value(), 256, "text/plain", "storage_unicode");

    REQUIRE(result.success());
    REQUIRE(result.value.value() > 0);
  }

  SECTION("Add multiple files to same folder")
  {
    auto file1_result = test_db->addFile("file1.txt", folder_id.value(), 100, "text/plain", "storage1");
    auto file2_result = test_db->addFile("file2.txt", folder_id.value(), 200, "text/plain", "storage2");
    auto file3_result = test_db->addFile("file3.txt", folder_id.value(), 300, "text/plain", "storage3");

    REQUIRE(file1_result.success());
    REQUIRE(file2_result.success());
    REQUIRE(file3_result.success());

    // All should be different
    REQUIRE(file1_result.value.value() != file2_result.value.value());
    REQUIRE(file2_result.value.value() != file3_result.value.value());
    REQUIRE(file1_result.value.value() != file3_result.value.value());
  }

  SECTION("Add files to different folders")
  {
    auto folder2_id = DatabaseTestHelper::createTestFolder(test_db.get(), "TestFolder2");

    auto file1_result = test_db->addFile("file1.txt", folder_id.value(), 100, "text/plain", "storage_f1");
    auto file2_result = test_db->addFile("file2.txt", folder2_id.value(), 200, "text/plain", "storage_f2");

    REQUIRE(file1_result.success());
    REQUIRE(file2_result.success());
    REQUIRE(file1_result.value.value() != file2_result.value.value());
  }

  SECTION("Duplicate storage ID should fail")
  {
    auto file1_result = test_db->addFile("file1.txt", folder_id.value(), 100, "text/plain", "duplicate_storage");
    REQUIRE(file1_result.success());

    auto file2_result = test_db->addFile("file2.txt", folder_id.value(), 200, "text/plain", "duplicate_storage");
    REQUIRE_FALSE(file2_result.success());
    REQUIRE(file2_result.error == DatabaseError::UniqueConstraint);
  }

  SECTION("Same filename different storage ID should succeed")
  {
    auto file1_result = test_db->addFile("same_name.txt", folder_id.value(), 100, "text/plain", "storage_a");
    auto file2_result = test_db->addFile("same_name.txt", folder_id.value(), 200, "text/plain", "storage_b");

    REQUIRE(file1_result.success());
    REQUIRE(file2_result.success());
    REQUIRE(file1_result.value.value() != file2_result.value.value());
  }

  SECTION("Add file to non-existent folder should fail")
  {
    auto result = test_db->addFile("test.txt", 99999, 100, "text/plain", "storage_nonexistent");

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::ForeignKeyConstraint);
    REQUIRE(result.errorMessage == "Folder doesn't exist");
  }

  SECTION("Add file with empty filename")
  {
    auto result = test_db->addFile("", folder_id.value(), 100, "text/plain", "storage_empty_name");

    REQUIRE(result.success()); // Empty filename should be allowed
  }

  SECTION("Add file with empty content type")
  {
    auto result = test_db->addFile("test.txt", folder_id.value(), 100, "", "storage_empty_type");

    REQUIRE(result.success()); // Empty content type should be allowed
  }

  SECTION("Add file with very long filename")
  {
    std::string long_name(1000, 'a'); // 1000 character filename
    long_name += ".txt";

    auto result = test_db->addFile(long_name, folder_id.value(), 100, "text/plain", "storage_long_name");

    REQUIRE(result.success());
  }

  SECTION("Add file with very long storage ID")
  {
    std::string long_storage_id(1000, 's'); // 1000 character storage ID

    auto result = test_db->addFile("test.txt", folder_id.value(), 100, "text/plain", long_storage_id);

    REQUIRE(result.success());
  }

  SECTION("Returned file ID is sequential")
  {
    auto file1_result = test_db->addFile("seq1.txt", folder_id.value(), 100, "text/plain", "storage_seq1");
    auto file2_result = test_db->addFile("seq2.txt", folder_id.value(), 200, "text/plain", "storage_seq2");
    auto file3_result = test_db->addFile("seq3.txt", folder_id.value(), 300, "text/plain", "storage_seq3");

    REQUIRE(file1_result.success());
    REQUIRE(file2_result.success());
    REQUIRE(file3_result.success());

    // IDs should be sequential (assuming no other operations)
    REQUIRE(file2_result.value.value() > file1_result.value.value());
    REQUIRE(file3_result.value.value() > file2_result.value.value());
  }
}

TEST_CASE("Database file operations edge cases", "[database][files][edge]")
{
  TestDatabase test_db("files_edge");

  SECTION("Add file without creating folder first")
  {
    // This should fail due to foreign key constraint
    auto result = test_db->addFile("orphan.txt", 1, 100, "text/plain", "storage_orphan");

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::ForeignKeyConstraint);
  }

  SECTION("Add file with negative folder ID")
  {
    auto result = test_db->addFile("negative.txt", -1, 100, "text/plain", "storage_negative");

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::ForeignKeyConstraint);
  }

  SECTION("Add file with negative size")
  {
    auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get());

    auto result = test_db->addFile("negative_size.txt", folder_id.value(), -100, "text/plain", "storage_neg_size");

    REQUIRE(result.success()); // SQLite allows negative integers
  }

  SECTION("Add many files quickly")
  {
    auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "BulkFolder");

    for (int i = 0; i < 100; ++i)
    {
      std::string filename = "bulk_file_" + std::to_string(i) + ".txt";
      std::string storage_id = "bulk_storage_" + std::to_string(i);

      auto result = test_db->addFile(filename, folder_id.value(), i * 10, "text/plain", storage_id);

      REQUIRE(result.success());
    }
  }

  SECTION("Test specific error types")
  {
    auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get());

    SECTION("Foreign key constraint error")
    {
      auto result = test_db->addFile("test.txt", 99999, 100, "text/plain", "storage_fk");

      REQUIRE_FALSE(result.success());
      REQUIRE(result.error == DatabaseError::ForeignKeyConstraint);
      REQUIRE(result.errorMessage == "Folder doesn't exist");
    }

    SECTION("Unique constraint error")
    {
      // First file should succeed
      auto result1 = test_db->addFile("file1.txt", folder_id.value(), 100, "text/plain", "duplicate_storage_id");
      REQUIRE(result1.success());

      // Second file with same storage_id should fail
      auto result2 = test_db->addFile("file2.txt", folder_id.value(), 200, "text/plain", "duplicate_storage_id");
      REQUIRE_FALSE(result2.success());
      REQUIRE(result2.error == DatabaseError::UniqueConstraint);
      REQUIRE(result2.errorMessage == "A file with this storage ID already exists");
    }
  }
}

TEST_CASE("Database file retrieval operations", "[database][files][retrieval]")
{
  TestDatabase test_db("file_retrieval");
  auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "RetrievalTest");

  SECTION("Get file by ID successfully")
  {
    auto add_result = test_db->addFile("test_retrieve.txt", folder_id.value(), 1024, "text/plain", "storage_retrieve");
    REQUIRE(add_result.success());
    auto file_id = add_result.value.value();

    auto get_result = test_db->getFileById(file_id);
    REQUIRE(get_result.success());

    auto file = get_result.value.value();
    REQUIRE(file.id == file_id);
    REQUIRE(file.name == "test_retrieve.txt");
    REQUIRE(file.folderId == folder_id.value());
    REQUIRE(file.size == 1024);
    REQUIRE(file.contentType == "text/plain");
    REQUIRE(file.storageId == "storage_retrieve");

    // Timestamps should be valid and recent
    auto now = std::chrono::system_clock::now();
    auto minute_ago = now - std::chrono::minutes(1);
    REQUIRE(file.createdAt >= minute_ago);
    REQUIRE(file.createdAt <= now);
    REQUIRE(file.updatedAt >= minute_ago);
    REQUIRE(file.updatedAt <= now);
  }

  SECTION("Get file by storage ID successfully")
  {
    auto add_result = test_db->addFile("storage_test.txt", folder_id.value(), 2048, "application/json", "unique_storage_id_123");
    REQUIRE(add_result.success());

    auto get_result = test_db->getFileByStorageId("unique_storage_id_123");
    REQUIRE(get_result.success());

    auto file = get_result.value.value();
    REQUIRE(file.id == add_result.value.value());
    REQUIRE(file.name == "storage_test.txt");
    REQUIRE(file.folderId == folder_id.value());
    REQUIRE(file.size == 2048);
    REQUIRE(file.contentType == "application/json");
    REQUIRE(file.storageId == "unique_storage_id_123");
  }

  SECTION("Get files by folder")
  {
    // Add multiple files to the folder
    auto file1_result = test_db->addFile("file1.txt", folder_id.value(), 100, "text/plain", "storage1");
    auto file2_result = test_db->addFile("file2.pdf", folder_id.value(), 200, "application/pdf", "storage2");
    auto file3_result = test_db->addFile("file3.jpg", folder_id.value(), 300, "image/jpeg", "storage3");

    REQUIRE(file1_result.success());
    REQUIRE(file2_result.success());
    REQUIRE(file3_result.success());

    // Create another folder with different files
    auto folder2_id = DatabaseTestHelper::createTestFolder(test_db.get(), "OtherFolder");
    auto other_file_result = test_db->addFile("other.txt", folder2_id.value(), 400, "text/plain", "storage_other");
    REQUIRE(other_file_result.success());

    // Get files from the first folder
    auto files_result = test_db->getFilesByFolder(folder_id.value());
    REQUIRE(files_result.success());

    auto files = files_result.value.value();
    REQUIRE(files.size() == 3);

    // Files should be ordered by name
    REQUIRE(files[0].name == "file1.txt");
    REQUIRE(files[1].name == "file2.pdf");
    REQUIRE(files[2].name == "file3.jpg");

    // Verify file details
    REQUIRE(files[0].folderId == folder_id.value());
    REQUIRE(files[1].folderId == folder_id.value());
    REQUIRE(files[2].folderId == folder_id.value());

    REQUIRE(files[0].storageId == "storage1");
    REQUIRE(files[1].storageId == "storage2");
    REQUIRE(files[2].storageId == "storage3");
  }

  SECTION("Get files from empty folder")
  {
    auto empty_folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "EmptyFolder");

    auto files_result = test_db->getFilesByFolder(empty_folder_id.value());
    REQUIRE(files_result.success());

    auto files = files_result.value.value();
    REQUIRE(files.empty());
  }

  SECTION("Get file by non-existent ID")
  {
    auto get_result = test_db->getFileById(99999);
    REQUIRE_FALSE(get_result.success());
    REQUIRE(get_result.error == DatabaseError::UnknownError);
    REQUIRE(get_result.errorMessage == "File not found");
  }

  SECTION("Get file by non-existent storage ID")
  {
    auto get_result = test_db->getFileByStorageId("non_existent_storage_id");
    REQUIRE_FALSE(get_result.success());
    REQUIRE(get_result.error == DatabaseError::UnknownError);
    REQUIRE(get_result.errorMessage == "File not found");
  }

  SECTION("Get files from non-existent folder")
  {
    auto files_result = test_db->getFilesByFolder(99999);
    REQUIRE(files_result.success());

    auto files = files_result.value.value();
    REQUIRE(files.empty());
  }
}

TEST_CASE("Database file update operations", "[database][files][update]")
{
  TestDatabase test_db("file_update");
  auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "UpdateTest");

  SECTION("Update file timestamp successfully")
  {
    auto add_result = test_db->addFile("timestamp_test.txt", folder_id.value(), 1024, "text/plain", "storage_timestamp");
    REQUIRE(add_result.success());
    auto file_id = add_result.value.value();

    // Get initial timestamps
    auto initial_file_result = test_db->getFileById(file_id);
    REQUIRE(initial_file_result.success());
    auto initial_file = initial_file_result.value.value();
    auto initial_updated_at = initial_file.updatedAt;

    // Wait a moment to ensure timestamp difference
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Update timestamp
    auto update_result = test_db->updateFileTimestamp(file_id);
    REQUIRE(update_result.success());
    REQUIRE(update_result.value.value() == true);

    // Verify timestamp was updated
    auto updated_file_result = test_db->getFileById(file_id);
    REQUIRE(updated_file_result.success());
    auto updated_file = updated_file_result.value.value();

    REQUIRE(updated_file.updatedAt > initial_updated_at);
    REQUIRE(updated_file.createdAt == initial_file.createdAt); // Created timestamp should not change
  }

  SECTION("Update timestamp for non-existent file")
  {
    auto update_result = test_db->updateFileTimestamp(99999);
    REQUIRE_FALSE(update_result.success());
    REQUIRE(update_result.error == DatabaseError::UnknownError);
    REQUIRE(update_result.errorMessage == "Failed to update file timestamp");
  }
}

TEST_CASE("Database file delete operations", "[database][files][delete]")
{
  TestDatabase test_db("file_delete");
  auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "DeleteTest");

  SECTION("Delete file successfully")
  {
    auto add_result = test_db->addFile("delete_me.txt", folder_id.value(), 1024, "text/plain", "storage_delete");
    REQUIRE(add_result.success());
    auto file_id = add_result.value.value();

    // Verify file exists
    auto get_result = test_db->getFileById(file_id);
    REQUIRE(get_result.success());

    // Delete the file
    auto delete_result = test_db->deleteFile(file_id);
    REQUIRE(delete_result.success());
    REQUIRE(delete_result.value.value() == true);

    // Verify file no longer exists
    auto get_after_delete = test_db->getFileById(file_id);
    REQUIRE_FALSE(get_after_delete.success());
    REQUIRE(get_after_delete.errorMessage == "File not found");
  }

  SECTION("Delete file and verify folder contents")
  {
    // Add multiple files
    auto file1_result = test_db->addFile("keep1.txt", folder_id.value(), 100, "text/plain", "storage_keep1");
    auto file2_result = test_db->addFile("delete.txt", folder_id.value(), 200, "text/plain", "storage_delete");
    auto file3_result = test_db->addFile("keep2.txt", folder_id.value(), 300, "text/plain", "storage_keep2");

    REQUIRE(file1_result.success());
    REQUIRE(file2_result.success());
    REQUIRE(file3_result.success());

    // Verify all files exist
    auto initial_files = test_db->getFilesByFolder(folder_id.value());
    REQUIRE(initial_files.success());
    REQUIRE(initial_files.value.value().size() == 3);

    // Delete the middle file
    auto delete_result = test_db->deleteFile(file2_result.value.value());
    REQUIRE(delete_result.success());
    REQUIRE(delete_result.value.value() == true);

    // Verify only 2 files remain
    auto remaining_files = test_db->getFilesByFolder(folder_id.value());
    REQUIRE(remaining_files.success());
    auto files = remaining_files.value.value();
    REQUIRE(files.size() == 2);

    // Verify correct files remain
    REQUIRE(files[0].name == "keep1.txt");
    REQUIRE(files[1].name == "keep2.txt");
  }

  SECTION("Delete non-existent file")
  {
    auto delete_result = test_db->deleteFile(99999);
    REQUIRE_FALSE(delete_result.success());
    REQUIRE(delete_result.error == DatabaseError::UnknownError);
    REQUIRE(delete_result.errorMessage == "Failed to delete file");
  }

  SECTION("Multiple deletes of same file")
  {
    auto add_result = test_db->addFile("multi_delete.txt", folder_id.value(), 1024, "text/plain", "storage_multi");
    REQUIRE(add_result.success());
    auto file_id = add_result.value.value();

    // First delete should succeed
    auto delete1_result = test_db->deleteFile(file_id);
    REQUIRE(delete1_result.success());
    REQUIRE(delete1_result.value.value() == true);

    // Second delete should fail
    auto delete2_result = test_db->deleteFile(file_id);
    REQUIRE_FALSE(delete2_result.success());
    REQUIRE(delete2_result.error == DatabaseError::UnknownError);
  }
}

TEST_CASE("Database file operations with special cases", "[database][files][special]")
{
  TestDatabase test_db("file_special");
  auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "SpecialTest");

  SECTION("Files with identical names in same folder")
  {
    // SQLite should allow duplicate filenames (only storage_id must be unique)
    auto file1_result = test_db->addFile("duplicate_name.txt", folder_id.value(), 100, "text/plain", "storage_dup1");
    auto file2_result = test_db->addFile("duplicate_name.txt", folder_id.value(), 200, "text/plain", "storage_dup2");

    REQUIRE(file1_result.success());
    REQUIRE(file2_result.success());
    REQUIRE(file1_result.value.value() != file2_result.value.value());

    // Both files should be retrievable
    auto files = test_db->getFilesByFolder(folder_id.value());
    REQUIRE(files.success());
    REQUIRE(files.value.value().size() == 2);
  }

  SECTION("File operations with various content types")
  {
    std::vector<std::pair<std::string, std::string>> test_cases = {
        {"document.pdf", "application/pdf"},
        {"image.png", "image/png"},
        {"video.mp4", "video/mp4"},
        {"audio.mp3", "audio/mpeg"},
        {"archive.zip", "application/zip"},
        {"data.json", "application/json"},
        {"style.css", "text/css"},
        {"script.js", "application/javascript"},
        {"unknown.xyz", "application/octet-stream"}};

    std::vector<int> file_ids;
    for (size_t i = 0; i < test_cases.size(); ++i)
    {
      auto &[filename, content_type] = test_cases[i];
      std::string storage_id = "storage_type_" + std::to_string(i);

      auto result = test_db->addFile(filename, folder_id.value(), static_cast<int>(i * 100), content_type, storage_id);
      REQUIRE(result.success());
      file_ids.push_back(result.value.value());
    }

    // Verify all files were created with correct content types
    for (size_t i = 0; i < file_ids.size(); ++i)
    {
      auto file_result = test_db->getFileById(file_ids[i]);
      REQUIRE(file_result.success());

      auto file = file_result.value.value();
      REQUIRE(file.name == test_cases[i].first);
      REQUIRE(file.contentType == test_cases[i].second);
      REQUIRE(file.size == static_cast<int>(i * 100));
    }
  }

  // TODO: change size type
  // SECTION("File operations with extreme sizes")
  // {
  //   // Zero size file
  //   auto zero_result = test_db->addFile("zero.txt", folder_id.value(), 0, "text/plain", "storage_zero");
  //   REQUIRE(zero_result.success());

  //   // Very large file (simulating 1TB)
  //   auto large_result = test_db->addFile("huge.bin", folder_id.value(), 1099511627776, "application/octet-stream", "storage_huge");
  //   REQUIRE(large_result.success());

  //   // Verify retrieval
  //   auto zero_file = test_db->getFileById(zero_result.value.value());
  //   auto large_file = test_db->getFileById(large_result.value.value());

  //   REQUIRE(zero_file.success());
  //   REQUIRE(large_file.success());

  //   REQUIRE(zero_file.value.value().size == 0);
  //   REQUIRE(large_file.value.value().size == 1099511627776);
  // }

  SECTION("File operations stress test")
  {
    // Create many files quickly
    const int NUM_FILES = 100;
    std::vector<int> file_ids;

    for (int i = 0; i < NUM_FILES; ++i)
    {
      std::string filename = "stress_file_" + std::to_string(i) + ".txt";
      std::string storage_id = "stress_storage_" + std::to_string(i);

      auto result = test_db->addFile(filename, folder_id.value(), i * 10, "text/plain", storage_id);
      REQUIRE(result.success());
      file_ids.push_back(result.value.value());
    }

    // Verify all files exist
    auto all_files = test_db->getFilesByFolder(folder_id.value());
    REQUIRE(all_files.success());
    REQUIRE(all_files.value.value().size() == NUM_FILES);

    // Update timestamps for half the files
    for (int i = 0; i < NUM_FILES / 2; ++i)
    {
      auto update_result = test_db->updateFileTimestamp(file_ids[i]);
      REQUIRE(update_result.success());
    }

    // Delete every other file
    for (int i = 0; i < NUM_FILES; i += 2)
    {
      auto delete_result = test_db->deleteFile(file_ids[i]);
      REQUIRE(delete_result.success());
    }

    // Verify correct number of files remain
    auto remaining_files = test_db->getFilesByFolder(folder_id.value());
    REQUIRE(remaining_files.success());
    REQUIRE(remaining_files.value.value().size() == NUM_FILES / 2);
  }

  SECTION("File retrieval by storage ID with special characters")
  {
    std::vector<std::string> special_storage_ids = {
        "storage-with-hyphens",
        "storage_with_underscores",
        "storage.with.dots",
        "storage123numbers",
        "UPPERCASE_STORAGE",
        "MixedCase_Storage-123.test"};

    std::vector<int> file_ids;
    for (size_t i = 0; i < special_storage_ids.size(); ++i)
    {
      std::string filename = "special_" + std::to_string(i) + ".txt";

      auto result = test_db->addFile(filename, folder_id.value(), static_cast<int>(i * 10), "text/plain", special_storage_ids[i]);
      REQUIRE(result.success());
      file_ids.push_back(result.value.value());
    }

    // Verify retrieval by storage ID
    for (size_t i = 0; i < special_storage_ids.size(); ++i)
    {
      auto file_result = test_db->getFileByStorageId(special_storage_ids[i]);
      REQUIRE(file_result.success());

      auto file = file_result.value.value();
      REQUIRE(file.id == file_ids[i]);
      REQUIRE(file.storageId == special_storage_ids[i]);
    }
  }
}

TEST_CASE("Database file operations integration with folders", "[database][files][folders][integration]")
{
  TestDatabase test_db("file_folder_integration");

  SECTION("Files are deleted when parent folder is deleted (CASCADE)")
  {
    // Create folder structure
    auto parent_folder_result = test_db->insertFolder("ParentForFiles");
    REQUIRE(parent_folder_result.success());
    auto parent_id = parent_folder_result.value.value();

    auto child_folder_result = test_db->insertFolder("ChildForFiles", parent_id);
    REQUIRE(child_folder_result.success());
    auto child_id = child_folder_result.value.value();

    // Add files to both folders
    auto parent_file = test_db->addFile("parent_file.txt", parent_id, 100, "text/plain", "storage_parent_file");
    auto child_file = test_db->addFile("child_file.txt", child_id, 200, "text/plain", "storage_child_file");

    REQUIRE(parent_file.success());
    REQUIRE(child_file.success());

    // Verify files exist
    REQUIRE(test_db->getFileById(parent_file.value.value()).success());
    REQUIRE(test_db->getFileById(child_file.value.value()).success());

    // Delete parent folder - should cascade delete child folder and both files
    DatabaseResult<bool> folder_deleted = test_db->deleteFolder(parent_id);
    REQUIRE(folder_deleted.value);

    // Verify files are gone (CASCADE DELETE)
    REQUIRE_FALSE(test_db->getFileById(parent_file.value.value()).success());
    REQUIRE_FALSE(test_db->getFileById(child_file.value.value()).success());

    // Verify folders are gone
    REQUIRE_FALSE(test_db->getFolderById(parent_id).success());
    REQUIRE_FALSE(test_db->getFolderById(child_id).success());
  }

  SECTION("Files in different folders are independent")
  {
    auto folder1_result = test_db->insertFolder("Independent1");
    auto folder2_result = test_db->insertFolder("Independent2");

    REQUIRE(folder1_result.success());
    REQUIRE(folder2_result.success());

    auto folder1_id = folder1_result.value.value();
    auto folder2_id = folder2_result.value.value();

    // Add files to both folders
    auto file1 = test_db->addFile("file1.txt", folder1_id, 100, "text/plain", "storage_indep1");
    auto file2 = test_db->addFile("file2.txt", folder2_id, 200, "text/plain", "storage_indep2");

    REQUIRE(file1.success());
    REQUIRE(file2.success());

    // Delete one folder
    DatabaseResult<bool> deleted = test_db->deleteFolder(folder1_id);
    REQUIRE(deleted.value);

    // File in deleted folder should be gone
    REQUIRE_FALSE(test_db->getFileById(file1.value.value()).success());

    // File in other folder should remain
    REQUIRE(test_db->getFileById(file2.value.value()).success());

    // Remaining folder should still exist
    REQUIRE(test_db->getFolderById(folder2_id).success());
    REQUIRE(test_db->getFolderById(folder2_id).value);
  }

  SECTION("Cannot add file to non-existent folder")
  {
    auto result = test_db->addFile("orphan.txt", 99999, 100, "text/plain", "storage_orphan");

    REQUIRE_FALSE(result.success());
    REQUIRE(result.error == DatabaseError::ForeignKeyConstraint);
    REQUIRE(result.errorMessage == "Folder doesn't exist");
  }
}