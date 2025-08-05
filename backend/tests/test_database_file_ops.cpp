#include <catch2/catch_test_macros.hpp>
#include "test_helpers_database.hpp"

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