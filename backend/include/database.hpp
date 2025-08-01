#include <string>
#include <vector>
#include <chrono>
#include <optional>

namespace bytebucket
{

  struct FileRecord
  {
    int id;
    std::string name;
    int folder_id;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    int size;
    std::string content_type;
    std::string storage_id;
  };

  struct FolderRecord
  {
    int id;
    std::string name;
    std::optional<int> parent_id;
  };
};