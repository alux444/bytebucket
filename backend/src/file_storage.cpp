#include "file_storage.hpp"
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>

namespace bytebucket
{

  std::optional<std::string> FileStorage::saveFile(
      const std::string &filename,
      const std::vector<char> &content,
      const std::string &content_type)
  {
    if (!initializeStorage())
    {
      return std::nullopt;
    }

    // Generate unique file ID
    std::string file_id = generateFileId();

    // Create file path
    std::filesystem::path storage_path = getStorageDir();
    std::filesystem::path file_path = storage_path / file_id;

    try
    {
      // Write file content
      std::ofstream file(file_path, std::ios::binary);
      if (!file.is_open())
      {
        return std::nullopt;
      }

      file.write(content.data(), content.size());
      file.close();

      // Create metadata file
      std::filesystem::path metadata_path = storage_path / (file_id + ".meta");
      std::ofstream metadata(metadata_path);
      if (metadata.is_open())
      {
        metadata << "original_filename=" << filename << "\n";
        metadata << "content_type=" << content_type << "\n";
        metadata << "size=" << content.size() << "\n";

        // Add timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        metadata << "uploaded_at=" << time_t << "\n";
        metadata.close();
      }

      return file_id;
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error saving file: " << e.what() << std::endl;
      return std::nullopt;
    }
  }

  std::optional<std::filesystem::path> FileStorage::getFilePath(const std::string &file_id)
  {
    std::filesystem::path storage_path = getStorageDir();
    std::filesystem::path file_path = storage_path / file_id;

    if (std::filesystem::exists(file_path))
    {
      return file_path;
    }

    return std::nullopt;
  }

  bool FileStorage::fileExists(const std::string &file_id)
  {
    std::filesystem::path storage_path = getStorageDir();
    std::filesystem::path file_path = storage_path / file_id;

    return std::filesystem::exists(file_path);
  }

  std::optional<std::vector<char>> FileStorage::readFile(const std::string &file_id)
  {
    if (!fileExists(file_id))
      return std::nullopt;

    std::filesystem::path storage_path = getStorageDir();
    std::filesystem::path file_path = storage_path / file_id;

    try
    {
      std::ifstream file(file_path, std::ios::binary);
      if (!file.is_open())
        return std::nullopt;

      file.seekg(0, std::ios::end);
      std::streamsize size = file.tellg();
      file.seekg(0, std::ios::beg);

      std::vector<char> content(size);
      if (!file.read(content.data(), size))
        return std::nullopt;

      return content;
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error reading file: " << e.what() << std::endl;
      return std::nullopt;
    }
  }

  std::string FileStorage::generateFileId()
  {
    // Generate a unique ID using timestamp + random number
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now.time_since_epoch())
                         .count();

    // Random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);

    std::stringstream ss;
    ss << std::hex << timestamp << "_" << dis(gen);

    return ss.str();
  }

  std::filesystem::path FileStorage::getStorageDir()
  {
    return STORAGE_DIR;
  }

  bool FileStorage::initializeStorage()
  {
    try
    {
      std::filesystem::path storage_path = getStorageDir();

      if (!std::filesystem::exists(storage_path))
      {
        return std::filesystem::create_directories(storage_path);
      }

      return std::filesystem::is_directory(storage_path);
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error initializing storage: " << e.what() << std::endl;
      return false;
    }
  }

} // namespace bytebucket
