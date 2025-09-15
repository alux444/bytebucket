#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace bytebucket
{

  class FileStorage
  {
  private:
    inline static const std::filesystem::path STORAGE_DIR = "storage";

  public:
    FileStorage() = delete;

    // Save file to storage directory and return unique file ID
    static std::optional<std::string> saveFile(
        const std::string &filename,
        const std::vector<char> &content,
        const std::string &content_type = "application/octet-stream");

    // Get file path by ID
    static std::optional<std::filesystem::path> getFilePath(const std::string &file_id);

    // Check if file exists
    static bool fileExists(const std::string &file_id);

    // Read file content by ID
    static std::optional<std::vector<char>> readFile(const std::string &file_id);

    // Delete file by ID (removes both content and metadata files)
    static bool deleteFile(const std::string &file_id);

    // Generate unique file ID
    static std::string generateFileId();

    // Get storage directory path
    static std::filesystem::path getStorageDir();

    // Initialize storage directory (create if doesn't exist)
    static bool initializeStorage();
  };

}
