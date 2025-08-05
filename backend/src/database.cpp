#include "database.hpp"
#include <iostream>

namespace bytebucket
{

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
      switch (returnCode)
      {
      case SQLITE_CONSTRAINT_FOREIGNKEY:
        result.error = DatabaseError::ForeignKeyConstraint;
        result.errorMessage = "Parent folder doesn't exist";
        break;
      case SQLITE_CONSTRAINT_NOTNULL:
        result.error = DatabaseError::NotNullConstraint;
        result.errorMessage = "File name cannot be empty";
        break;
      case SQLITE_CONSTRAINT_UNIQUE:
        result.error = DatabaseError::UniqueConstraint;
        result.errorMessage = "A folder with this storage ID already exists in the parent directory";
        break;
      case SQLITE_CONSTRAINT:
        result.error = DatabaseError::UnknownError;
        result.errorMessage = "Constraint violation: " + std::string(sqlite3_errmsg(db.get()));
        break;
      default:
        result.error = DatabaseError::UnknownError;
        result.errorMessage = "Database error: " + std::string(sqlite3_errmsg(db.get()));
        break;
      }
      return result;
    }

    result.value = static_cast<int>(sqlite3_last_insert_rowid(db.get()));
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
      switch (returnCode)
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
        result.errorMessage = "Constraint violation: " + std::string(sqlite3_errmsg(db.get()));
        break;
      default:
        result.error = DatabaseError::UnknownError;
        result.errorMessage = "Database error: " + std::string(sqlite3_errmsg(db.get()));
        break;
      }
      return result;
    }

    result.value = static_cast<int>(sqlite3_last_insert_rowid(db.get()));
    result.error = DatabaseError::Success;
    return result;
  }

  std::optional<FolderRecord> Database::getFolderById(int id) const
  {
    const char *sql = R"(
      SELECT id, name, parent_id 
      FROM folders 
      WHERE id = ?
    )";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v3(db.get(), sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, nullptr) != SQLITE_OK)
      return std::nullopt;

    sqlite3_bind_int(stmt, 1, id);

    int returnCode = sqlite3_step(stmt);
    if (returnCode != SQLITE_ROW)
    {
      sqlite3_finalize(stmt);
      return std::nullopt;
    }

    FolderRecord folder;
    folder.id = sqlite3_column_int(stmt, 0);
    folder.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

    if (sqlite3_column_type(stmt, 2) == SQLITE_NULL)
      folder.parentId = std::nullopt;
    else
      folder.parentId = sqlite3_column_int(stmt, 2);

    sqlite3_finalize(stmt);
    return folder;
  }

  std::vector<FolderRecord> Database::getFoldersByParent(std::optional<int> parentId) const
  {
    std::vector<FolderRecord> folders;
    const char *sql = R"(
      SELECT id, name, parent_id 
      FROM folders 
      WHERE parent_id = ? OR (parent_id IS NULL and ? IS NULL)
      ORDER BY name
    )";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v3(db.get(), sql, -1, SQLITE_PREPARE_PERSISTENT, &stmt, nullptr) != SQLITE_OK)
      return folders;

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
    return folders;
  }

  bool Database::deleteFolder(int id)
  // TODO: either migrate db for cascades or do recursive delete solution
  {
    const char *deleteSql = R"(
      DELETE FROM folders 
      WHERE id = ?
    )";
    sqlite3_stmt *deleteStmt = nullptr;

    if (sqlite3_prepare_v3(db.get(), deleteSql, -1, SQLITE_PREPARE_PERSISTENT, &deleteStmt, nullptr) != SQLITE_OK)
      return false;

    sqlite3_bind_int(deleteStmt, 1, id);

    int returnCode = sqlite3_step(deleteStmt);
    sqlite3_finalize(deleteStmt);

    return returnCode == SQLITE_DONE && sqlite3_changes(db.get()) > 0;
  }
#pragma endregion folders

}