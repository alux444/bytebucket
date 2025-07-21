#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace bytebucket
{

  struct MultipartFile
  {
    std::string name;          // Field name
    std::string filename;      // Original filename
    std::string content_type;  // MIME type
    std::vector<char> content; // File content
  };

  struct MultipartField
  {
    std::string name;  // Field name
    std::string value; // Field value
  };

  struct MultipartData
  {
    std::vector<MultipartFile> files;
    std::vector<MultipartField> fields;
  };

  class MultipartParser
  {
  public:
    static std::optional<MultipartData> parse(const std::string &body, const std::string &boundary);

  private:
    static std::string extractBoundary(const std::string &content_type);
    static std::unordered_map<std::string, std::string> parseHeaders(const std::string &header_section);
    static std::string trim(const std::string &str);
  };

}
