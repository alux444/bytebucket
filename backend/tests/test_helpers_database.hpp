#pragma once

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <memory>
#include "database.hpp"

namespace bytebucket
{
  namespace test
  {

    class DatabaseTestHelper
    {
    public:
      // Create a test database with a unique name
      static std::shared_ptr<Database> createTestDatabase(const std::string &test_name)
      {
        std::string test_db_path = "test_db_" + test_name + ".db";

        // Clean up any existing test database
        cleanupDatabase(test_db_path);

        auto db = Database::create(test_db_path);
        REQUIRE(db != nullptr);

        return db;
      }

      // Clean up database files
      static void cleanupDatabase(const std::string &db_path)
      {
        std::filesystem::remove(db_path);
        std::filesystem::remove(db_path + "-wal");
        std::filesystem::remove(db_path + "-shm");
      }

      // RAII wrapper for automatic cleanup
      class TestDatabase
      {
      public:
        explicit TestDatabase(const std::string &test_name)
            : db_path("test_db_" + test_name + ".db")
        {
          DatabaseTestHelper::cleanupDatabase(db_path);
          db = Database::create(db_path);
          REQUIRE(db != nullptr);
        }

        ~TestDatabase()
        {
          db.reset();
          DatabaseTestHelper::cleanupDatabase(db_path);
        }

        // Non-copyable, non-movable
        TestDatabase(const TestDatabase &) = delete;
        TestDatabase &operator=(const TestDatabase &) = delete;
        TestDatabase(TestDatabase &&) = delete;
        TestDatabase &operator=(TestDatabase &&) = delete;

        std::shared_ptr<Database> operator->() const { return db; }
        std::shared_ptr<Database> get() const { return db; }

      private:
        std::string db_path;
        std::shared_ptr<Database> db;
      };

      // Helper to create a test folder
      static std::optional<int> createTestFolder(std::shared_ptr<Database> db,
                                                 const std::string &name = "TestFolder")
      {
        auto folder_id = db->insertFolder(name);
        REQUIRE(folder_id.has_value());
        return folder_id;
      }

      // Helper to create multiple test folders
      static std::vector<int> createTestFolders(std::shared_ptr<Database> db,
                                                const std::vector<std::string> &names)
      {
        std::vector<int> folder_ids;
        for (const auto &name : names)
        {
          auto folder_id = createTestFolder(db, name);
          folder_ids.push_back(folder_id.value());
        }
        return folder_ids;
      }

      // Helper to create a test file
      static std::optional<int> createTestFile(std::shared_ptr<Database> db,
                                               int folder_id,
                                               const std::string &name = "test.txt",
                                               int size = 1024,
                                               const std::string &content_type = "text/plain",
                                               const std::string &storage_id = "storage123")
      {
        auto file_id = db->addFile(name, folder_id, size, content_type, storage_id);
        REQUIRE(file_id.has_value());
        return file_id;
      }
    };

  } // namespace test
} // namespace bytebucket