#include "database.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace bytebucket
{
  inline std::optional<std::chrono::system_clock::time_point> parseSqliteToChrono(const char *sqlite3Time)
  {
    if (!sqlite3Time)
      return std::nullopt;

    std::string timeString(sqlite3Time);
    if (timeString.empty())
      return std::nullopt;

    int FORMAT_LENGTH = 19; // "YYYY-MM-DD HH:MM:SS"
    if (timeString.length() != FORMAT_LENGTH)
      return std::nullopt;

    if (timeString[4] != '-' || timeString[7] != '-' || timeString[10] != ' ' ||
        timeString[13] != ':' || timeString[16] != ':')
      return std::nullopt;

    for (int i = 0; i < 19; ++i)
    {
      if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16)
        continue; // skip separators
      if (!std::isdigit(timeString[i]))
        return std::nullopt;
    }

    std::tm tm{};
    std::istringstream ss{sqlite3Time};
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    if (ss.fail() || !ss.eof())
      return std::nullopt;

    bool isValidYear = tm.tm_year >= 0;
    bool isValidMonth = tm.tm_mon >= 0 && tm.tm_mon <= 11;
    bool isValidDay = tm.tm_mday >= 1 && tm.tm_mday <= 31;
    bool isValidHour = tm.tm_hour >= 0 && tm.tm_hour <= 23;
    bool isValidMinute = tm.tm_min >= 0 && tm.tm_min <= 59;
    bool isValidSecond = tm.tm_sec >= 0 && tm.tm_sec <= 59;

    if (!isValidYear || !isValidMonth || !isValidDay || !isValidHour || !isValidMinute || !isValidSecond)
      return std::nullopt;

    if (tm.tm_mon == 1)
    { // 1 = feb
      int year = tm.tm_year + 1900;
      bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
      int maxDays = isLeapYear ? 29 : 28;
      if (tm.tm_mday > maxDays)
        return std::nullopt;
    }
    else if (tm.tm_mon == 3 || tm.tm_mon == 5 || tm.tm_mon == 8 || tm.tm_mon == 10)
    { // April, June, September, November
      if (tm.tm_mday > 30)
        return std::nullopt;
    }

    char *originalTimezone = getenv("TZ");
    setenv("TZ", "UTC", 1);
    tzset();

    time_t utcTime = mktime(&tm);

    if (originalTimezone)
      setenv("TZ", originalTimezone, 1);
    else
      unsetenv("TZ");
    tzset();

    if (utcTime == -1)
      return std::nullopt;

    return std::chrono::system_clock::from_time_t(utcTime);
  }

  std::shared_ptr<Database> Database::create(const std::string &dbPath)
  {
    sqlite3 *db = nullptr;
    int returnCode = sqlite3_open(dbPath.c_str(), &db);
    if (returnCode != SQLITE_OK)
    {
      std::cerr << "Couldn't open database: " << sqlite3_errmsg(db) << std::endl;
      if (db)
        sqlite3_close(db);
      return nullptr;
    }

    auto database = std::shared_ptr<Database>(new Database(db));
    if (!database->executePragma() || !database->executeSchema())
      return nullptr;

    return database;
  }

  Database::Database(sqlite3 *db) : db(db) {}
  Database::~Database() = default;

  bool Database::executePragma() const
  {
    const char *pragmas[] = {
        "PRAGMA foreign_keys = ON;",
        "PRAGMA defer_foreign_keys = OFF;",
        "PRAGMA journal_mode = WAL;", // write ahead logging
        "PRAGMA synchronous = NORMAL;"};

    for (const char *pragma : pragmas)
    {
      char *errMsg = nullptr;
      int returnCode = sqlite3_exec(db.get(), pragma, nullptr, nullptr, &errMsg);
      if (returnCode != SQLITE_OK)
      {
        std::cerr << "Pragma error: " << errMsg << std::endl;
        sqlite3_free(errMsg); // free actual string assigned by the C string allocated
        return false;
      }
    }
    return true;
  }

  bool Database::executeSchema() const
  {
    const char *schema = R"(
      CREATE TABLE IF NOT EXISTS folders (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        parent_id INTEGER,
        FOREIGN KEY (parent_id) REFERENCES folders(id) ON DELETE CASCADE,
        UNIQUE (name, parent_id)
      );

      CREATE TABLE IF NOT EXISTS files (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        folder_id INTEGER NOT NULL,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP,
        updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
        size INTEGER,
        content_type TEXT,
        storage_id TEXT UNIQUE NOT NULL,
        FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE CASCADE
      );

      CREATE INDEX IF NOT EXISTS idx_files_folder_id ON files(folder_id);
      CREATE INDEX IF NOT EXISTS idx_folders_parent_id ON folders(parent_id);
      CREATE INDEX IF NOT EXISTS idx_folder_name ON folders(name);
      CREATE INDEX IF NOT EXISTS idx_files_name ON files(name);
      CREATE INDEX IF NOT EXISTS idx_files_content_type ON files(content_type);
      CREATE INDEX IF NOT EXISTS idx_files_storage_id ON files(storage_id);

      CREATE TABLE IF NOT EXISTS tags (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL UNIQUE
      );

      CREATE INDEX IF NOT EXISTS idx_tags_name ON tags(name);

      CREATE TABLE IF NOT EXISTS file_tags (
        file_id INTEGER NOT NULL,
        tag_id INTEGER NOT NULL,
        PRIMARY KEY (file_id, tag_id),
        FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE,
        FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
      );

      CREATE TABLE IF NOT EXISTS file_metadata (
        file_id INTEGER NOT NULL,
        key TEXT NOT NULL,
        value TEXT,
        PRIMARY KEY (file_id, key),
        FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE
      );
    )";

    char *errMsg = nullptr;
    int returnCode = sqlite3_exec(db.get(), schema, nullptr, nullptr, &errMsg);
    if (returnCode != SQLITE_OK)
    {
      std::cerr << "Schema error: " << errMsg << std::endl;
      sqlite3_free(errMsg);
      return false;
    }
    return true;
  }

#pragma region files
  DatabaseResult<int> Database::addFile(
      std::string_view name,
      int folderId,
      int size,
      std::string_view contentType,
      std::string_view storageId)
  {
    DatabaseResult<int> result;
    const char *sql = R"(
      INSERT INTO files (name, folder_id, created_at, updated_at, size, content_type, storage_id) 
      VALUES (?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, ?, ?, ?)
    )";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v3(db.get(), sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, nullptr) != SQLITE_OK)
    {
      result.error = DatabaseError::PrepareStatementFailed;
      result.errorMessage = "Failed to prepare file insert statement";
      return result;
    }

    sqlite3_bind_text(stmt, 1, name.data(), static_cast<int>(name.size()), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, folderId);
    sqlite3_bind_int(stmt, 3, size);
    sqlite3_bind_text(stmt, 4, contentType.data(), static_cast<int>(contentType.size()), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, storageId.data(), static_cast<int>(storageId.size()), SQLITE_STATIC);

    int returnCode = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (returnCode != SQLITE_DONE)
    {
      int extendedErrorCode = sqlite3_extended_errcode(db.get());
      std::string errorMsg = sqlite3_errmsg(db.get());

      switch (extendedErrorCode)
      {
      case SQLITE_CONSTRAINT_FOREIGNKEY:
        result.error = DatabaseError::ForeignKeyConstraint;
        result.errorMessage = "Folder doesn't exist";
        break;
      case SQLITE_CONSTRAINT_NOTNULL:
        result.error = DatabaseError::NotNullConstraint;
        result.errorMessage = "File name cannot be empty";
        break;
      case SQLITE_CONSTRAINT_UNIQUE:
        result.error = DatabaseError::UniqueConstraint;
        result.errorMessage = "A file with this storage ID already exists";
        break;
      case SQLITE_CONSTRAINT:
        result.error = DatabaseError::UnknownError;
        result.errorMessage = "Constraint violation: " + errorMsg;
        break;
      default:
        result.error = DatabaseError::UnknownError;
        result.errorMessage = "Database error: " + errorMsg;
        break;
      }
      return result;
    }

    result.value = static_cast<int>(sqlite3_last_insert_rowid(db.get()));
    result.error = DatabaseError::Success;
    return result;
  }

  DatabaseResult<FileRecord> Database::getFileById(int id) const
  {
    DatabaseResult<FileRecord> result;
    const char *sql = R"(
      SELECT id, name, folder_id, created_at, updated_at, size, content_type, storage_id 
      FROM files 
      WHERE id = ?
    )";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v3(db.get(), sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, nullptr) != SQLITE_OK)
    {
      result.error = DatabaseError::PrepareStatementFailed;
      result.errorMessage = "Failed to prepare folder insert statement";
      return result;
    }

    sqlite3_bind_int(stmt, 1, id);

    int returnCode = sqlite3_step(stmt);
    if (returnCode != SQLITE_ROW)
    {
      sqlite3_finalize(stmt);
      result.error = DatabaseError::UnknownError;
      result.errorMessage = "File not found";
      return result;
    }

    FileRecord file;
    file.id = sqlite3_column_int(stmt, 0);
    file.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    file.folderId = sqlite3_column_int(stmt, 2);
    file.createdAt = parseSqliteToChrono(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3))).value();
    file.updatedAt = parseSqliteToChrono(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4))).value();
    file.size = sqlite3_column_int(stmt, 5);
    file.contentType = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
    file.storageId = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));

    sqlite3_finalize(stmt);
    result.value = file;
    result.error = DatabaseError::Success;
    return result;
  }

  DatabaseResult<FileRecord> Database::getFileByStorageId(std::string_view storageId) const
  {
    DatabaseResult<FileRecord> result;
    const char *sql = R"(
      SELECT id, name, folder_id, created_at, updated_at, size, content_type, storage_id 
      FROM files 
      WHERE storage_id = ?
    )";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v3(db.get(), sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, nullptr) != SQLITE_OK)
    {
      result.error = DatabaseError::PrepareStatementFailed;
      result.errorMessage = "Failed to prepare folder insert statement";
      return result;
    }

    sqlite3_bind_text(stmt, 1, storageId.data(), static_cast<int>(storageId.size()), SQLITE_STATIC);

    int returnCode = sqlite3_step(stmt);
    if (returnCode != SQLITE_ROW)
    {
      sqlite3_finalize(stmt);
      result.error = DatabaseError::UnknownError;
      result.errorMessage = "File not found";
      return result;
    }

    FileRecord file;
    file.id = sqlite3_column_int(stmt, 0);
    file.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    file.folderId = sqlite3_column_int(stmt, 2);
    file.createdAt = parseSqliteToChrono(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3))).value();
    file.updatedAt = parseSqliteToChrono(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4))).value();
    file.size = sqlite3_column_int(stmt, 5);
    file.contentType = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
    file.storageId = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));

    sqlite3_finalize(stmt);
    result.value = file;
    result.error = DatabaseError::Success;
    return result;
  }

  DatabaseResult<std::vector<FileRecord>> Database::getFilesByFolder(int folderId) const
  {
    DatabaseResult<std::vector<FileRecord>> result;
    const char *sql = R"(
      SELECT id, name, folder_id, created_at, updated_at, size, content_type, storage_id 
      FROM files 
      WHERE folder_id = ?
      ORDER BY name
    )";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v3(db.get(), sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, nullptr) != SQLITE_OK)
    {
      result.error = DatabaseError::PrepareStatementFailed;
      result.errorMessage = "Failed to prepare files by folder statement";
      return result;
    }

    sqlite3_bind_int(stmt, 1, folderId);

    std::vector<FileRecord> files;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
      FileRecord file;
      file.id = sqlite3_column_int(stmt, 0);
      file.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      file.folderId = sqlite3_column_int(stmt, 2);
      file.createdAt = parseSqliteToChrono(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3))).value();
      file.updatedAt = parseSqliteToChrono(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4))).value();
      file.size = sqlite3_column_int(stmt, 5);
      file.contentType = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
      file.storageId = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));

      files.push_back(std::move(file));
    }

    sqlite3_finalize(stmt);
    result.value = std::move(files);
    result.error = DatabaseError::Success;
    return result;
  }

  DatabaseResult<bool> Database::updateFileTimestamp(int id)
  {
    DatabaseResult<bool> result;
    const char *sql = R"(
      UPDATE files 
      SET updated_at = CURRENT_TIMESTAMP 
      WHERE id = ?
    )";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v3(db.get(), sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, nullptr) != SQLITE_OK)
    {
      result.error = DatabaseError::PrepareStatementFailed;
      result.errorMessage = "Failed to prepare update file timestamp statement";
      return result;
    }

    sqlite3_bind_int(stmt, 1, id);

    int returnCode = sqlite3_step(stmt);
    if (returnCode != SQLITE_DONE)
    {
      sqlite3_finalize(stmt);
      result.error = DatabaseError::UnknownError;
      result.errorMessage = "Failed to update file timestamp";
      return result;
    }

    int changes = sqlite3_changes(db.get());
    if (changes == 0)
    {
      sqlite3_finalize(stmt);
      result.error = DatabaseError::UnknownError;
      result.errorMessage = "Failed to update file timestamp";
      return result;
    }

    result.value = sqlite3_changes(db.get()) > 0;
    result.error = DatabaseError::Success;
    return result;
  }

  DatabaseResult<bool> Database::deleteFile(int id)
  {
    DatabaseResult<bool> result;
    const char *sql = R"(
      DELETE FROM files 
      WHERE id = ?
    )";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v3(db.get(), sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, nullptr) != SQLITE_OK)
    {
      result.error = DatabaseError::PrepareStatementFailed;
      result.errorMessage = "Failed to prepare delete file statement";
      return result;
    }

    sqlite3_bind_int(stmt, 1, id);

    int returnCode = sqlite3_step(stmt);
    if (returnCode != SQLITE_DONE)
    {
      sqlite3_finalize(stmt);
      result.error = DatabaseError::UnknownError;
      result.errorMessage = "Failed to delete file";
      return result;
    }

    int changes = sqlite3_changes(db.get());
    if (changes == 0)
    {
      sqlite3_finalize(stmt);
      result.error = DatabaseError::UnknownError;
      result.errorMessage = "Failed to delete file";
      return result;
    }

    result.value = sqlite3_changes(db.get()) > 0;
    result.error = DatabaseError::Success;
    return result;
  }

#pragma endregion files

#pragma region folders
  DatabaseResult<int> Database::insertFolder(std::string_view name, std::optional<int> parentId)
  {
    DatabaseResult<int> result;
    const char *sql = R"(
      INSERT INTO folders (name, parent_id) 
      VALUES (?, ?)
    )";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v3(db.get(), sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, nullptr) != SQLITE_OK)
    {
      result.error = DatabaseError::PrepareStatementFailed;
      result.errorMessage = "Failed to prepare folder insert statement";
      return result;
    }

    sqlite3_bind_text(stmt, 1, name.data(), static_cast<int>(name.size()), SQLITE_STATIC);
    if (parentId.has_value())
      sqlite3_bind_int(stmt, 2, parentId.value());
    else
      sqlite3_bind_null(stmt, 2);

    int returnCode = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (returnCode != SQLITE_DONE)
    {
      int extendedErrorCode = sqlite3_extended_errcode(db.get());
      std::string errorMsg = sqlite3_errmsg(db.get());

      switch (extendedErrorCode)
      {
      case SQLITE_CONSTRAINT_FOREIGNKEY:
        result.error = DatabaseError::ForeignKeyConstraint;
        result.errorMessage = "Parent folder doesn't exist";
        break;
      case SQLITE_CONSTRAINT_NOTNULL:
        result.error = DatabaseError::NotNullConstraint;
        result.errorMessage = "Folder name cannot be empty";
        break;
      case SQLITE_CONSTRAINT_UNIQUE:
        result.error = DatabaseError::UniqueConstraint;
        result.errorMessage = "A folder with this name already exists in the parent directory";
        break;
      case SQLITE_CONSTRAINT:
        result.error = DatabaseError::UnknownError;
        result.errorMessage = "Constraint violation: " + errorMsg;
        break;
      default:
        result.error = DatabaseError::UnknownError;
        result.errorMessage = "Database error: " + errorMsg;
        break;
      }
      return result;
    }

    result.value = static_cast<int>(sqlite3_last_insert_rowid(db.get()));
    result.error = DatabaseError::Success;
    return result;
  }

  DatabaseResult<FolderRecord> Database::getFolderById(int id) const
  {
    DatabaseResult<FolderRecord> result;
    const char *sql = R"(
      SELECT id, name, parent_id 
      FROM folders 
      WHERE id = ?
    )";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v3(db.get(), sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, nullptr) != SQLITE_OK)
    {
      result.error = DatabaseError::PrepareStatementFailed;
      result.errorMessage = "Failed to prepare fetch folder statement";
      return result;
    }

    sqlite3_bind_int(stmt, 1, id);

    int returnCode = sqlite3_step(stmt);
    if (returnCode != SQLITE_ROW)
    {
      sqlite3_finalize(stmt);
      result.error = DatabaseError::UnknownError;
      result.errorMessage = "Folder not found";
      return result;
    }

    FolderRecord folder;
    folder.id = sqlite3_column_int(stmt, 0);
    folder.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

    if (sqlite3_column_type(stmt, 2) == SQLITE_NULL)
      folder.parentId = std::nullopt;
    else
      folder.parentId = sqlite3_column_int(stmt, 2);

    sqlite3_finalize(stmt);
    result.value = folder;
    result.error = DatabaseError::Success;
    return result;
  }

  DatabaseResult<std::vector<FolderRecord>> Database::getFoldersByParent(std::optional<int> parentId) const
  {
    DatabaseResult<std::vector<FolderRecord>> result;
    std::vector<FolderRecord> folders;
    const char *sql = R"(
      SELECT id, name, parent_id 
      FROM folders 
      WHERE parent_id = ? OR (parent_id IS NULL and ? IS NULL)
      ORDER BY name
    )";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v3(db.get(), sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, nullptr) != SQLITE_OK)
    {
      result.error = DatabaseError::PrepareStatementFailed;
      result.errorMessage = "Failed to prepare fetch folders statement";
      return result;
    }

    if (parentId.has_value())
    {
      sqlite3_bind_int(stmt, 1, parentId.value());
      sqlite3_bind_int(stmt, 2, parentId.value());
    }
    else
    {
      sqlite3_bind_null(stmt, 1);
      sqlite3_bind_null(stmt, 2);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
      FolderRecord folder;
      folder.id = sqlite3_column_int(stmt, 0);
      folder.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

      if (sqlite3_column_type(stmt, 2) == SQLITE_NULL)
        folder.parentId = std::nullopt;
      else
        folder.parentId = sqlite3_column_int(stmt, 2);

      folders.push_back(std::move(folder));
    }

    sqlite3_finalize(stmt);
    result.value = folders;
    result.error = DatabaseError::Success;
    return result;
  }

  DatabaseResult<bool> Database::deleteFolder(int id)
  {
    DatabaseResult<bool> result;
    const char *deleteSql = R"(
      DELETE FROM folders 
      WHERE id = ?
    )";
    sqlite3_stmt *deleteStmt = nullptr;

    if (sqlite3_prepare_v3(db.get(), deleteSql, -1, SQLITE_PREPARE_PERSISTENT, &deleteStmt, nullptr) != SQLITE_OK)
    {
      result.error = DatabaseError::PrepareStatementFailed;
      result.errorMessage = "Failed to prepare fetch folders statement";
      return result;
    }

    sqlite3_bind_int(deleteStmt, 1, id);

    int returnCode = sqlite3_step(deleteStmt);
    sqlite3_finalize(deleteStmt);

    if (returnCode != SQLITE_DONE)
    {
      result.error = DatabaseError::UnknownError;
      result.errorMessage = "Failed to execute delete folder query";
      return result;
    }

    if (sqlite3_changes(db.get()) == 0)
    {
      result.value = false;
      result.error = DatabaseError::UnknownError;
      result.errorMessage = "DELETE action resulted in no changes";
      return result;
    }

    result.value = true;
    result.error = DatabaseError::Success;
    return result;
  }
#pragma endregion folders

}