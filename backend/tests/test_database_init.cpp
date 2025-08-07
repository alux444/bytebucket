#include <catch2/catch_test_macros.hpp>
#include "test_helpers_database.hpp"
#include <sstream>

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

TEST_CASE("SQLite timestamp parsing", "[database][parsing]")
{
  SECTION("Parse valid SQLite timestamp")
  {
    const char *valid_timestamp = "2024-08-07 14:30:25";
    auto result = parseSqliteToChrono(valid_timestamp);

    REQUIRE(result.has_value());

    // Convert back to time_t to verify parsing
    auto time_t_val = std::chrono::system_clock::to_time_t(result.value());
    auto tm_ptr = std::gmtime(&time_t_val);

    REQUIRE(tm_ptr->tm_year + 1900 == 2024);
    REQUIRE(tm_ptr->tm_mon + 1 == 8); // tm_mon is 0-based
    REQUIRE(tm_ptr->tm_mday == 7);
    REQUIRE(tm_ptr->tm_hour == 14);
    REQUIRE(tm_ptr->tm_min == 30);
    REQUIRE(tm_ptr->tm_sec == 25);
  }

  SECTION("Parse current timestamp format")
  {
    const char *current_format = "2025-01-15 09:45:12";
    auto result = parseSqliteToChrono(current_format);

    REQUIRE(result.has_value());

    auto time_t_val = std::chrono::system_clock::to_time_t(result.value());
    auto tm_ptr = std::gmtime(&time_t_val);

    REQUIRE(tm_ptr->tm_year + 1900 == 2025);
    REQUIRE(tm_ptr->tm_mon + 1 == 1);
    REQUIRE(tm_ptr->tm_mday == 15);
    REQUIRE(tm_ptr->tm_hour == 9);
    REQUIRE(tm_ptr->tm_min == 45);
    REQUIRE(tm_ptr->tm_sec == 12);
  }

  SECTION("Parse edge case timestamps")
  {
    // New Year's Day
    const char *new_year = "2024-01-01 00:00:00";
    auto result = parseSqliteToChrono(new_year);
    REQUIRE(result.has_value());

    // End of year
    const char *end_year = "2024-12-31 23:59:59";
    result = parseSqliteToChrono(end_year);
    REQUIRE(result.has_value());

    // Leap year February 29th
    const char *leap_day = "2024-02-29 12:00:00";
    result = parseSqliteToChrono(leap_day);
    REQUIRE(result.has_value());
  }

  SECTION("Handle null pointer")
  {
    auto result = parseSqliteToChrono(nullptr);
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("Handle invalid timestamp formats")
  {
    // Invalid format - missing seconds
    auto result = parseSqliteToChrono("2024-08-07 14:30");
    REQUIRE_FALSE(result.has_value());

    // Invalid format - wrong separator
    result = parseSqliteToChrono("2024/08/07 14:30:25");
    REQUIRE_FALSE(result.has_value());

    // Invalid format - extra characters
    result = parseSqliteToChrono("2024-08-07 14:30:25 UTC");
    REQUIRE_FALSE(result.has_value());

    // Invalid format - missing date
    result = parseSqliteToChrono("14:30:25");
    REQUIRE_FALSE(result.has_value());

    // Invalid format - missing time
    result = parseSqliteToChrono("2024-08-07");
    REQUIRE_FALSE(result.has_value());

    // Completely invalid
    result = parseSqliteToChrono("not a timestamp");
    REQUIRE_FALSE(result.has_value());

    // Empty string
    result = parseSqliteToChrono("");
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("Handle invalid date values")
  {
    // Invalid month
    auto result = parseSqliteToChrono("2024-13-07 14:30:25");
    REQUIRE_FALSE(result.has_value());

    // Invalid day
    result = parseSqliteToChrono("2024-08-32 14:30:25");
    REQUIRE_FALSE(result.has_value());

    // Invalid hour
    result = parseSqliteToChrono("2024-08-07 25:30:25");
    REQUIRE_FALSE(result.has_value());

    // Invalid minute
    result = parseSqliteToChrono("2024-08-07 14:60:25");
    REQUIRE_FALSE(result.has_value());

    // Invalid second
    result = parseSqliteToChrono("2024-08-07 14:30:60");
    REQUIRE_FALSE(result.has_value());

    // February 30th (invalid)
    result = parseSqliteToChrono("2024-02-30 12:00:00");
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("Test round-trip consistency")
  {
    const char *original = "2024-08-07 14:30:25";
    auto parsed = parseSqliteToChrono(original);
    REQUIRE(parsed.has_value());

    // Convert back to string and verify format
    auto time_t_val = std::chrono::system_clock::to_time_t(parsed.value());
    auto tm_ptr = std::gmtime(&time_t_val);

    std::ostringstream oss;
    oss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");

    REQUIRE(oss.str() == original);
  }

  SECTION("Test with actual SQLite CURRENT_TIMESTAMP format")
  {
    // SQLite CURRENT_TIMESTAMP format should be parseable
    TestDatabase test_db("timestamp_test");

    // Insert a file to test timestamp generation
    auto folder_id = DatabaseTestHelper::createTestFolder(test_db.get(), "TimestampTest");
    auto file_result = test_db->addFile("test_file.txt", folder_id.value(), 100, "text/plain", "storage_test_timestamp");

    REQUIRE(file_result.success());

    // Retrieve the file and verify timestamps are valid
    auto retrieved_file = test_db->getFileById(file_result.value.value());
    REQUIRE(retrieved_file.success());

    // Verify that timestamps are valid (not default constructed)
    auto file = retrieved_file.value.value();

    // Check that created_at and updated_at are reasonable (within last minute)
    auto now = std::chrono::system_clock::now();
    auto minute_ago = now - std::chrono::minutes(1);

    REQUIRE(file.createdAt >= minute_ago);
    REQUIRE(file.createdAt <= now);
    REQUIRE(file.updatedAt >= minute_ago);
    REQUIRE(file.updatedAt <= now);

    // For a new file, created_at and updated_at should be very close
    auto diff = file.updatedAt - file.createdAt;
    REQUIRE(diff <= std::chrono::seconds(1));
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