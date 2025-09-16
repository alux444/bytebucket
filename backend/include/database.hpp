#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <memory>
#include <sqlite3.h>

namespace bytebucket
{
  std::optional<std::chrono::system_clock::time_point> parseSqliteToChrono(const char *sqlite3Time);

  struct FileRecord
  {
    int id;
    std::string name;
    int folderId;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point updatedAt;
    int size;
    std::string contentType;
    std::string storageId; // id in local storage folder
  };

  struct FolderRecord
  {
    int id;
    std::string name;
    std::optional<int> parentId;
  };

  enum class DatabaseError
  {
    Success,
    ForeignKeyConstraint,
    NotNullConstraint,
    UniqueConstraint,
    PrepareStatementFailed,
    UnknownError
  };

  template <typename T>
  struct DatabaseResult
  {
    std::optional<T> value;
    DatabaseError error = DatabaseError::Success;
    std::string errorMessage;

    bool success() const { return error == DatabaseError::Success; }
    explicit operator bool() const { return success(); }
  };

  class Database
  {
  public:
    static std::shared_ptr<Database> create(const std::string &dbPath = "bytebucket.db");

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;
    Database(Database &&) = delete;
    Database &operator=(Database &&) = delete;

    ~Database();

    // files
    DatabaseResult<int> addFile(
        std::string_view name,
        int folderId,
        int size,
        std::string_view contentType,
        std::string_view storageId);
    DatabaseResult<FileRecord> getFileById(int id) const;
    DatabaseResult<FileRecord> getFileByStorageId(std::string_view storageId) const;
    DatabaseResult<std::vector<FileRecord>> getFilesByFolder(int folderId) const;
    DatabaseResult<bool> updateFileTimestamp(int id);
    DatabaseResult<bool> deleteFile(int id);
    DatabaseResult<bool> renameFile(int id, std::string_view name);
    DatabaseResult<bool> moveFile(int id, int parentId);

    // folders
    DatabaseResult<int> insertFolder(std::string_view name, std::optional<int> parentId = std::nullopt);
    DatabaseResult<FolderRecord> getFolderById(int id) const;
    DatabaseResult<std::vector<FolderRecord>> getFoldersByParent(std::optional<int> parentId) const;
    DatabaseResult<bool> deleteFolder(int id);
    DatabaseResult<bool> renameFolder(int id, std::string_view name);
    DatabaseResult<bool> moveFolder(int id, int parentId);

    // tags
    DatabaseResult<int> insertTag(std::string_view name);
    DatabaseResult<int> getTagByName(std::string_view name) const;
    DatabaseResult<std::string> getTagById(int id) const;
    DatabaseResult<std::vector<std::string>> getAllTags() const;
    DatabaseResult<bool> addFileTag(int fileId, int tagId);
    DatabaseResult<bool> removeFileTag(int fileId, int tagId);
    DatabaseResult<std::vector<std::string>> getFileTags(int fileId) const;

    // metadata
    DatabaseResult<bool> setFileMetadata(int fileId, std::string_view key, std::string_view value);
    DatabaseResult<std::string> getFileMetadata(int fileId, std::string_view key) const;
    DatabaseResult<std::vector<std::pair<std::string, std::string>>> getAllFileMetadata(int fileId) const;

  private:
    explicit Database(sqlite3 *db);

    struct SQLiteDeleter
    {
      void operator()(sqlite3 *db)
      {
        if (db)
          sqlite3_close(db);
      }
    };

    std::unique_ptr<sqlite3, SQLiteDeleter> db;
    // when it's destroyed, it should call SQLiteDeleter::operator()(sqlite3*)

    bool executeSchema() const;
    bool executePragma() const;
  };
}