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
        FOREIGN KEY (parent_id) REFERENCES folders(id) ON DELETE SET NULL
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

}