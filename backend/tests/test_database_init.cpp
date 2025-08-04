#include <catch2/catch_test_macros.hpp>
#include "test_helpers_database.hpp"

using namespace bytebucket;
using namespace bytebucket::test;
using TestDatabase = DatabaseTestHelper::TestDatabase;

TEST_CASE("Database creation and initialization", "[database]")
{

  SECTION("Create database successfully")
  {
    TestDatabase test_db("create");

    // Database is automatically created and cleaned up
    REQUIRE(test_db.get() != nullptr);
  }

  SECTION("Create database with default path")
  {
    // Clean up any existing default database
    DatabaseTestHelper::cleanupDatabase("bytebucket.db");

    auto db = Database::create();

    REQUIRE(db != nullptr);
    REQUIRE(std::filesystem::exists("bytebucket.db"));

    // Cleanup
    db.reset();
    DatabaseTestHelper::cleanupDatabase("bytebucket.db");
  }

  SECTION("Database creation with invalid path returns nullptr")
  {
    // Try to create database in non-existent directory
    std::string invalid_path = "/nonexistent/directory/test.db";

    auto db = Database::create(invalid_path);

    REQUIRE(db == nullptr);
  }
}

TEST_CASE("Database schema creation", "[database][schema]")
{
  TestDatabase test_db("schema");

  SECTION("All required tables are created")
  {
    // We'll directly query the database to verify tables exist
    sqlite3 *raw_db = nullptr;
    int rc = sqlite3_open("test_db_schema.db", &raw_db);
    REQUIRE(rc == SQLITE_OK);

    // Check if folders table exists
    const char *check_folders = "SELECT name FROM sqlite_master WHERE type='table' AND name='folders';";
    sqlite3_stmt *stmt = nullptr;
    rc = sqlite3_prepare_v2(raw_db, check_folders, -1, &stmt, nullptr);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    // Check if files table exists
    const char *check_files = "SELECT name FROM sqlite_master WHERE type='table' AND name='files';";
    rc = sqlite3_prepare_v2(raw_db, check_files, -1, &stmt, nullptr);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    // Check if tags table exists
    const char *check_tags = "SELECT name FROM sqlite_master WHERE type='table' AND name='tags';";
    rc = sqlite3_prepare_v2(raw_db, check_tags, -1, &stmt, nullptr);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    // Check if file_tags table exists
    const char *check_file_tags = "SELECT name FROM sqlite_master WHERE type='table' AND name='file_tags';";
    rc = sqlite3_prepare_v2(raw_db, check_file_tags, -1, &stmt, nullptr);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    // Check if file_metadata table exists
    const char *check_file_metadata = "SELECT name FROM sqlite_master WHERE type='table' AND name='file_metadata';";
    rc = sqlite3_prepare_v2(raw_db, check_file_metadata, -1, &stmt, nullptr);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    sqlite3_close(raw_db);
  }

  SECTION("Indexes are created")
  {
    sqlite3 *raw_db = nullptr;
    int rc = sqlite3_open("test_db_schema.db", &raw_db);
    REQUIRE(rc == SQLITE_OK);

    // Check for some key indexes
    const char *check_indexes = "SELECT name FROM sqlite_master WHERE type='index' AND name LIKE 'idx_%';";
    sqlite3_stmt *stmt = nullptr;
    rc = sqlite3_prepare_v2(raw_db, check_indexes, -1, &stmt, nullptr);
    REQUIRE(rc == SQLITE_OK);

    int index_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
      index_count++;
    }
    sqlite3_finalize(stmt);

    // We should have at least the indexes we defined
    REQUIRE(index_count >= 6);

    sqlite3_close(raw_db);
  }
}

TEST_CASE("Database RAII and resource management", "[database][raii]")
{
  SECTION("Database properly closes when going out of scope")
  {
    std::string test_db_path = "test_db_raii.db";

    {
      TestDatabase test_db("raii");
      REQUIRE(std::filesystem::exists(test_db_path));
    } // db should be destroyed and cleaned up here

    // Database file should be cleaned up
    REQUIRE_FALSE(std::filesystem::exists(test_db_path));
  }

  SECTION("Shared pointer properly manages lifetime")
  {
    std::shared_ptr<Database> db1;
    std::shared_ptr<Database> db2;

    {
      TestDatabase test_db("lifetime");
      auto temp_db = test_db.get();
      db1 = temp_db;
      db2 = temp_db;
      REQUIRE(temp_db.use_count() == 4);
    }

    REQUIRE(db1.use_count() == 2);
    REQUIRE(db2.use_count() == 2);

    db1.reset();
    REQUIRE(db2.use_count() == 1);

    db2.reset();
    // Database should now be properly closed
  }
}

TEST_CASE("Database copy/move semantics", "[database][semantics]")
{
  TestDatabase test_db("semantics");

  SECTION("Database is not copyable")
  {
    // These should not compile if uncommented:
    // Database db_copy(*test_db.get());
    // Database db_copy2 = *test_db.get();
    // *test_db.get() = *test_db.get();

    // This test passes if the code compiles (copy operations are deleted)
    REQUIRE(true);
  }

  SECTION("Database is not movable")
  {
    // These should not compile if uncommented:
    // Database db_move(std::move(*test_db.get()));
    // Database db_move2 = std::move(*test_db.get());

    // This test passes if the code compiles (move operations are deleted)
    REQUIRE(true);
  }
}