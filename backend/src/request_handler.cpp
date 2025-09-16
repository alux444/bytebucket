#include "request_handler.hpp"
#include "multipart_parser.hpp"
#include "file_storage.hpp"
#include "database.hpp"
#include <boost/beast/http.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace bytebucket
{
  const std::string SERVER_NAME{"ByteBucket-Server"};

  template <typename T>
  void addCorsHeaders(boost::beast::http::response<T> &res)
  {
    // TODO: specific frontend access control urls
    res.set(boost::beast::http::field::access_control_allow_origin, "*");
    res.set(boost::beast::http::field::access_control_allow_methods, "GET, POST, DELETE, OPTIONS");
    res.set(boost::beast::http::field::access_control_allow_headers, "Content-Type");
  }

  boost::beast::http::response<boost::beast::http::string_body>
  create_error_response(boost::beast::http::status status, unsigned version, const std::string &error_message)
  {
    boost::beast::http::response<boost::beast::http::string_body> res{status, version};
    res.set(boost::beast::http::field::server, SERVER_NAME);
    res.set(boost::beast::http::field::content_type, "application/json");
    addCorsHeaders(res);
    res.body() = R"({"error":")" + error_message + R"("})";
    res.prepare_payload();
    return res;
  }

  boost::beast::http::response<boost::beast::http::string_body>
  create_success_response(boost::beast::http::status status, unsigned version, const std::string &content_type, const std::string &body)
  {
    boost::beast::http::response<boost::beast::http::string_body> res{status, version};
    res.set(boost::beast::http::field::server, SERVER_NAME);
    res.set(boost::beast::http::field::content_type, content_type);
    addCorsHeaders(res);
    res.body() = body;
    res.prepare_payload();
    return res;
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_options(unsigned version)
  {
    boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, version};
    res.set(boost::beast::http::field::server, SERVER_NAME);
    addCorsHeaders(res);
    res.prepare_payload();
    return res;
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_health(unsigned version)
  {
    return create_success_response(boost::beast::http::status::ok, version, "application/json", R"({"status":"ok"})");
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_root(unsigned version)
  {
    return create_success_response(boost::beast::http::status::ok, version, "text/plain", "ByteBucket");
  }

  void buildFileJson(std::ostringstream &json_stream, const FileRecord &file, std::shared_ptr<Database> db)
  {
    auto created_time_t = std::chrono::system_clock::to_time_t(file.createdAt);
    auto updated_time_t = std::chrono::system_clock::to_time_t(file.updatedAt);

    std::ostringstream created_ss, updated_ss;
    created_ss << std::put_time(std::gmtime(&created_time_t), "%Y-%m-%dT%H:%M:%SZ");
    updated_ss << std::put_time(std::gmtime(&updated_time_t), "%Y-%m-%dT%H:%M:%SZ");

    json_stream << R"({"id":)" << file.id
                << R"(,"name":")" << file.name << R"(")"
                << R"(,"folder_id":)" << file.folderId
                << R"(,"size":)" << file.size
                << R"(,"content_type":")" << file.contentType << R"(")"
                << R"(,"created_at":")" << created_ss.str() << R"(")"
                << R"(,"updated_at":")" << updated_ss.str() << R"(")"
                << R"(,"storage_id":")" << file.storageId << R"(")";

    auto tags_result = db->getFileTags(file.id);
    json_stream << R"(,"tags":[)";
    if (tags_result.success() && tags_result.value.has_value())
    {
      const auto &tags = *tags_result.value;
      for (size_t j = 0; j < tags.size(); ++j)
      {
        if (j > 0)
          json_stream << ",";
        json_stream << R"(")" << tags[j] << R"(")";
      }
    }
    json_stream << "]";

    auto metadata_result = db->getAllFileMetadata(file.id);
    json_stream << R"(,"metadata":{)";
    if (metadata_result.success() && metadata_result.value.has_value())
    {
      const auto &metadata = *metadata_result.value;
      for (size_t j = 0; j < metadata.size(); ++j)
      {
        if (j > 0)
          json_stream << ",";
        json_stream << R"(")" << metadata[j].first << R"(":")" << metadata[j].second << R"(")";
      }
    }
    json_stream << "}";

    json_stream << "}";
  }

  boost::beast::http::response<boost::beast::http::string_body> handle_get_folder(const boost::beast::http::request<boost::beast::http::string_body> &req)
  {
    auto db = Database::create();
    if (!db)
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Failed to initialize database");

    std::string target = std::string(req.target());
    std::optional<int> folder_id;

    if (target.length() > 8 && target.substr(0, 8) == "/folder/")
    {
      std::string folder_id_str = target.substr(8); // Remove "/folder/" prefix
      if (!folder_id_str.empty())
      {
        try
        {
          folder_id = std::stoi(folder_id_str);
        }
        catch (const std::exception &)
        {
          return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                       "Invalid folder ID");
        }
      }
      // If folder_id_str is empty (i.e., request was "/folder/"), treat as root folder
    }
    // For "/folder" or "/folder/" without ID, folder_id remains std::nullopt (root folder)

    if (folder_id.has_value())
    {
      auto folder_result = db->getFolderById(folder_id.value());
      if (!folder_result.success())
      {
        return create_error_response(boost::beast::http::status::not_found, req.version(),
                                     "Folder not found");
      }
    }

    auto subfolders_result = db->getFoldersByParent(folder_id);
    if (!subfolders_result.success())
    {
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Failed to retrieve subfolders");
    }

    DatabaseResult<std::vector<FileRecord>> files_result;
    if (folder_id.has_value())
    {
      files_result = db->getFilesByFolder(folder_id.value());
    }
    else
    {
      auto root_folders = db->getFoldersByParent(std::nullopt);
      if (root_folders.success() && !root_folders.value->empty())
      {
        int root_id = root_folders.value->front().id;
        files_result = db->getFilesByFolder(root_id);
      }
      else
      {
        files_result.value = std::vector<FileRecord>{};
        files_result.error = DatabaseError::Success;
      }
    }

    if (!files_result.success())
    {
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Failed to retrieve files");
    }

    std::ostringstream json_response;
    json_response << "{";

    if (folder_id.has_value())
    {
      auto folder_result = db->getFolderById(folder_id.value());
      if (folder_result.success())
      {
        json_response << R"("folder":{"id":)" << folder_result.value->id
                      << R"(,"name":")" << folder_result.value->name << R"(")"
                      << R"(,"parentId":)";
        if (folder_result.value->parentId.has_value())
          json_response << folder_result.value->parentId.value();
        else
          json_response << "null";
        json_response << "},";
      }
    }
    else
    {
      json_response << R"("folder":{"id":null,"name":"root","parentId":null},)";
    }

    json_response << R"("subfolders":[)";
    for (size_t i = 0; i < subfolders_result.value->size(); ++i)
    {
      const auto &folder = (*subfolders_result.value)[i];
      if (i > 0)
        json_response << ",";
      json_response << R"({"id":)" << folder.id
                    << R"(,"name":")" << folder.name << R"(")"
                    << R"(,"parentId":)";
      if (folder.parentId.has_value())
        json_response << folder.parentId.value();
      else
        json_response << "null";
      json_response << "}";
    }
    json_response << "],";

    json_response << R"("files":[)";
    for (size_t i = 0; i < files_result.value->size(); ++i)
    {
      const auto &file = (*files_result.value)[i];
      if (i > 0)
        json_response << ",";

      buildFileJson(json_response, file, db);
    }
    json_response << "]";

    json_response << "}";

    return create_success_response(boost::beast::http::status::ok, req.version(),
                                   "application/json", json_response.str());
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_get_tags(const boost::beast::http::request<boost::beast::http::string_body> &req)
  {
    auto db = Database::create();
    if (!db)
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Failed to initialize database");

    auto tags_result = db->getAllTags();
    if (!tags_result.success())
    {
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Failed to retrieve tags: " + tags_result.errorMessage);
    }

    std::ostringstream json_response;
    json_response << R"({"tags":[)";

    const auto &tags = *tags_result.value;
    for (size_t i = 0; i < tags.size(); ++i)
    {
      if (i > 0)
        json_response << ",";
      json_response << R"(")" << tags[i] << R"(")";
    }

    json_response << "]}";

    return create_success_response(boost::beast::http::status::ok, req.version(),
                                   "application/json", json_response.str());
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_post_tags(const boost::beast::http::request<boost::beast::http::string_body> &req)
  {
    auto content_type_it = req.find(boost::beast::http::field::content_type);
    if (content_type_it == req.end() ||
        content_type_it->value().find("application/json") == std::string::npos)
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Content-Type must be application/json");

    std::string body = req.body();
    if (body.empty())
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Request body is required");

    // Parse JSON to extract tag name
    // Expected format: {"name": "tag_name"}
    size_t name_pos = body.find("\"name\"");
    if (name_pos == std::string::npos)
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Missing 'name' field in request body");

    size_t colon_pos = body.find(":", name_pos);
    size_t quote_start = body.find("\"", colon_pos + 1);
    size_t quote_end = body.find("\"", quote_start + 1);

    if (quote_start == std::string::npos || quote_end == std::string::npos)
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid JSON format for 'name' field");

    std::string tag_name = body.substr(quote_start + 1, quote_end - quote_start - 1);
    if (tag_name.empty())
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Tag name cannot be empty");

    auto db = Database::create();
    if (!db)
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Database connection failed");

    DatabaseResult<int> dbResult = db->insertTag(tag_name);
    if (!dbResult.success())
    {
      if (dbResult.errorMessage.find("already exists") != std::string::npos)
        return create_error_response(boost::beast::http::status::conflict, req.version(),
                                     "Tag already exists");
      else
        return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                     "Failed to create tag: " + dbResult.errorMessage);
    }

    std::ostringstream json_response;
    json_response << R"({"id":)" << dbResult.value.value()
                  << R"(,"name":")" << tag_name << R"("})";

    return create_success_response(boost::beast::http::status::created, req.version(),
                                   "application/json", json_response.str());
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_post_file_tags(const boost::beast::http::request<boost::beast::http::string_body> &req)
  {
    auto content_type_it = req.find(boost::beast::http::field::content_type);
    if (content_type_it == req.end() ||
        content_type_it->value().find("application/json") == std::string::npos)
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Content-Type must be application/json");

    std::string target = std::string(req.target());
    // Expected format: /files/{fileId}/tags
    if (target.length() <= 7 || target.substr(0, 7) != "/files/")
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid URL format");

    size_t tags_pos = target.find("/tags");
    if (tags_pos == std::string::npos)
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid URL format");

    std::string file_id_str = target.substr(7, tags_pos - 7); // Extract between "/files/" and "/tags"
    if (file_id_str.empty())
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "File ID is required");

    int file_id;
    try
    {
      file_id = std::stoi(file_id_str);
    }
    catch (const std::exception &)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid file ID format");
    }

    std::string body = req.body();
    if (body.empty())
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Request body is required");

    // Parse JSON to extract tag name
    // Expected format: {"tagName": "tag_name"}
    std::optional<int> tag_id;

    size_t tag_name_pos = body.find("\"tagName\"");

    auto db = Database::create();
    if (!db)
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Database connection failed");

    if (tag_name_pos != std::string::npos)
    {
      // Extract tag name and get/create tag ID
      size_t colon_pos = body.find(":", tag_name_pos);
      size_t quote_start = body.find("\"", colon_pos + 1);
      size_t quote_end = body.find("\"", quote_start + 1);

      if (quote_start == std::string::npos || quote_end == std::string::npos)
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "Invalid JSON format for 'tagName' field");

      std::string tag_name = body.substr(quote_start + 1, quote_end - quote_start - 1);
      if (tag_name.empty())
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "Tag name cannot be empty");

      // Try to get existing tag by name
      auto existing_tag = db->getTagByName(tag_name);
      if (existing_tag.success() && existing_tag.value.has_value())
      {
        tag_id = existing_tag.value.value();
      }
      else
      {
        // Create new tag if it doesn't exist
        auto new_tag = db->insertTag(tag_name);
        if (!new_tag.success())
          return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                       "Failed to create tag: " + new_tag.errorMessage);
        tag_id = new_tag.value.value();
      }
    }
    else
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Either 'tagName' field is required");
    }

    auto file_result = db->getFileById(file_id);
    if (!file_result.success() || !file_result.value.has_value())
      return create_error_response(boost::beast::http::status::not_found, req.version(),
                                   "File not found");

    auto add_result = db->addFileTag(file_id, tag_id.value());
    if (!add_result.success())
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Failed to add tag to file: " + add_result.errorMessage);

    std::ostringstream json_response;
    buildFileJson(json_response, file_result.value.value(), db);

    return create_success_response(boost::beast::http::status::ok, req.version(),
                                   "application/json", json_response.str());
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_post_file_metadata(const boost::beast::http::request<boost::beast::http::string_body> &req)
  {
    auto content_type_it = req.find(boost::beast::http::field::content_type);
    if (content_type_it == req.end() ||
        content_type_it->value().find("application/json") == std::string::npos)
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Content-Type must be application/json");

    std::string target = std::string(req.target());
    // Expected format: /files/{fileId}/metadata
    if (target.length() <= 7 || target.substr(0, 7) != "/files/")
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid URL format");

    size_t metadata_pos = target.find("/metadata");
    if (metadata_pos == std::string::npos)
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid URL format");

    std::string file_id_str = target.substr(7, metadata_pos - 7); // Extract between "/files/" and "/metadata"
    if (file_id_str.empty())
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "File ID is required");

    int file_id;
    try
    {
      file_id = std::stoi(file_id_str);
    }
    catch (const std::exception &)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid file ID format");
    }

    std::string body = req.body();
    if (body.empty())
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Request body is required");

    auto db = Database::create();
    if (!db)
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Database connection failed");

    auto file_result = db->getFileById(file_id);
    if (!file_result.success() || !file_result.value.has_value())
      return create_error_response(boost::beast::http::status::not_found, req.version(),
                                   "File not found");

    // Expected format: {"key1": "value1", "key2": "value2", ...}
    size_t pos = 0;
    bool any_metadata_added = false;

    while (pos < body.length())
    {
      // Find next key
      size_t key_start = body.find("\"", pos);
      if (key_start == std::string::npos)
        break;

      size_t key_end = body.find("\"", key_start + 1);
      if (key_end == std::string::npos)
        break;

      std::string key = body.substr(key_start + 1, key_end - key_start - 1);

      size_t colon_pos = body.find(":", key_end);
      if (colon_pos == std::string::npos)
        break;

      size_t value_start = body.find("\"", colon_pos);
      if (value_start == std::string::npos)
        break;
      size_t value_end = body.find("\"", value_start + 1);
      if (value_end == std::string::npos)
        break;

      std::string value = body.substr(value_start + 1, value_end - value_start - 1);

      if (key.empty())
      {
        pos = value_end + 1;
        continue;
      }

      auto set_result = db->setFileMetadata(file_id, key, value);
      if (!set_result.success())
        return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                     "Failed to set metadata: " + set_result.errorMessage);

      any_metadata_added = true;
      pos = value_end + 1;
    }

    if (!any_metadata_added)
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "No valid metadata key-value pairs found in request");

    std::ostringstream json_response;
    buildFileJson(json_response, file_result.value.value(), db);

    return create_success_response(boost::beast::http::status::ok, req.version(),
                                   "application/json", json_response.str());
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_post_folder(const boost::beast::http::request<boost::beast::http::string_body> &req)
  {
    auto content_type_it = req.find(boost::beast::http::field::content_type);
    if (content_type_it == req.end() ||
        content_type_it->value().find("application/json") == std::string::npos)
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Content-Type must be application/json");

    std::string body = req.body();
    std::string folder_name;
    std::optional<int> parent_id;

    size_t name_pos = body.find("\"name\"");
    if (name_pos == std::string::npos)
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Missing 'name' field in JSON");

    size_t colon_pos = body.find(":", name_pos);
    size_t quote_start = body.find("\"", colon_pos);
    size_t quote_end = body.find("\"", quote_start + 1);
    if (colon_pos == std::string::npos || quote_start == std::string::npos || quote_end == std::string::npos)
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid 'name' field in JSON");

    folder_name = body.substr(quote_start + 1, quote_end - quote_start - 1);
    if (folder_name.empty())
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Folder name can't be empty");

    size_t parent_pos = body.find("\"parent_id\"");
    if (parent_pos != std::string::npos)
    {
      size_t colon_pos = body.find(":", parent_pos);
      size_t number_start = body.find_first_of("0123456789", colon_pos + 1);
      size_t number_end = body.find_first_not_of("0123456789", number_start);
      try
      {
        parent_id = std::stoi(body.substr(number_start, number_end - number_start));
      }
      catch (...)
      {
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "Failed to parse parent_id. Expected argument is integer with no quotes, otherwise omitted for no parent id.");
      }
    }

    auto db = Database::create();
    if (!db)
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Database connection failed");
    DatabaseResult<int> dbResult = db->insertFolder(folder_name, parent_id);
    if (!dbResult.success() || !dbResult.value.has_value())
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   dbResult.errorMessage);

    std::ostringstream response_json;
    response_json << R"({"id":)" << *dbResult.value
                  << R"(,"name":")" << folder_name << R"(")";

    if (parent_id.has_value())
      response_json << R"(,"parent_id":)" << parent_id.value();
    else
      response_json << R"(,"parent_id":null)";

    response_json << "}";

    return create_success_response(boost::beast::http::status::created, req.version(),
                                   "application/json", response_json.str());
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_post_upload(const boost::beast::http::request<boost::beast::http::string_body> &req)
  {
    auto content_type_it = req.find(boost::beast::http::field::content_type);
    if (content_type_it == req.end())
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Content-Type header is required");

    std::string content_type = std::string(content_type_it->value());
    if (content_type.find("multipart/form-data") == std::string::npos)
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Content-Type should be multipart/form-data");

    std::string boundary = MultipartParser::extractBoundary(content_type);
    if (boundary.empty())
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid boundary in Content-Type");

    auto multipart_data = MultipartParser::parse(req.body(), boundary);
    if (!multipart_data.has_value())
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Failed to parse multipart data");

    if (multipart_data->files.empty())
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "No files found in request");

    auto db = Database::create();
    if (!db)
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Database connection failed");

    std::optional<int> folder_id;
    for (const auto &field : multipart_data->fields)
    {
      if (field.name == "folder_id")
      {
        try
        {
          folder_id = std::stoi(field.value);
        }
        catch (...)
        {
          return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                       "Invalid folder_id");
        }
        break;
      }
    }

    if (!folder_id.has_value())
    {
      auto root_folders = db->getFoldersByParent(std::nullopt);
      if (root_folders.success() && !root_folders.value->empty())
      {
        folder_id = root_folders.value->front().id;
      }
      else
      {
        auto root_folder_result = db->insertFolder("root", std::nullopt);
        if (root_folder_result.success() && root_folder_result.value.has_value())
        {
          folder_id = root_folder_result.value.value();
        }
        else
        {
          return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                       "Failed to create root folder");
        }
      }
    }

    std::ostringstream response_json;
    response_json << R"({"files":[)";
    bool first_file = true;

    for (const auto &file : multipart_data->files)
    {
      auto storage_id = FileStorage::saveFile(file.filename, file.content, file.content_type);
      if (!storage_id.has_value())
        return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                     "Failed to save file to storage");

      auto db_result = db->addFile(
          file.filename,
          folder_id.value(),
          static_cast<int>(file.content.size()),
          file.content_type,
          storage_id.value());

      if (!db_result.success() || !db_result.value.has_value())
      {
        // TODO: Add FileStorage::deleteFile method
        return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                     "Failed to save file to database: " + db_result.errorMessage);
      }

      if (!first_file)
        response_json << ",";
      first_file = false;

      auto file_record_result = db->getFileById(db_result.value.value());
      if (file_record_result.success() && file_record_result.value.has_value())
      {
        buildFileJson(response_json, file_record_result.value.value(), db);
      }
      else
      {
        return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                     "Error with fetching file after saving to db" + file_record_result.errorMessage);
      }
    }

    response_json << "]}";

    return create_success_response(boost::beast::http::status::ok, req.version(),
                                   "application/json", response_json.str());
  }

  boost::beast::http::response<boost::beast::http::vector_body<char>>
  create_binary_response(boost::beast::http::status status, unsigned version,
                         const std::string &content_type, const std::string &filename,
                         const std::vector<char> &content)
  {
    boost::beast::http::response<boost::beast::http::vector_body<char>> res{status, version};
    res.set(boost::beast::http::field::server, SERVER_NAME);
    res.set(boost::beast::http::field::content_type, content_type);
    res.set(boost::beast::http::field::content_disposition, "attachment; filename=\"" + filename + "\"");
    addCorsHeaders(res);
    res.body() = content;
    res.prepare_payload();
    return res;
  }

  boost::beast::http::message_generator
  handle_get_download(const boost::beast::http::request<boost::beast::http::string_body> &req)
  {
    std::string target = std::string(req.target());
    std::string file_id_str = target.substr(10); // Remove "/download/" prefix

    if (file_id_str.empty())
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "File ID is required");

    int file_id;
    try
    {
      file_id = std::stoi(file_id_str);
    }
    catch (...)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid file ID format");
    }

    auto db = Database::create();
    if (!db)
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Database connection failed");

    auto db_result = db->getFileById(file_id);
    if (!db_result.success() || !db_result.value.has_value())
      return create_error_response(boost::beast::http::status::not_found, req.version(),
                                   "File not found");

    const FileRecord &file_record = db_result.value.value();
    auto file_content = FileStorage::readFile(file_record.storageId);
    if (!file_content.has_value())
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Failed to read file from storage");

    return create_binary_response(boost::beast::http::status::ok, req.version(),
                                  file_record.contentType, file_record.name, file_content.value());
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_delete_file(const boost::beast::http::request<boost::beast::http::string_body> &req)
  {
    std::string target = std::string(req.target());

    // /files/{fileId}
    if (target.length() <= 7 || target.substr(0, 7) != "/files/")
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid file endpoint");
    }

    std::string file_id_str = target.substr(7); // Remove "/files/"
    if (file_id_str.empty())
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "File ID is required");
    }

    int file_id;
    try
    {
      file_id = std::stoi(file_id_str);
    }
    catch (...)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid file ID format");
    }

    auto db = Database::create();
    if (!db)
    {
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Database connection failed");
    }

    auto db_result = db->getFileById(file_id);
    if (!db_result.success() || !db_result.value.has_value())
    {
      return create_error_response(boost::beast::http::status::not_found, req.version(),
                                   "File not found");
    }

    const FileRecord &file_record = db_result.value.value();
    bool storage_deleted = FileStorage::deleteFile(file_record.storageId);
    if (!storage_deleted)
    {
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Failed to delete file from storage");
    }

    auto delete_result = db->deleteFile(file_id);
    if (!delete_result.success() || !delete_result.value.has_value() || !delete_result.value.value())
    {
      // TODO: make this case not reachable (existing in db but not in storage)
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Failed to delete file from database");
    }

    return create_success_response(boost::beast::http::status::ok, req.version(),
                                   "application/json", R"({"message":"File deleted successfully"})");
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_delete_folder(const boost::beast::http::request<boost::beast::http::string_body> &req)
  {
    std::string target = std::string(req.target());

    // Extract folder ID from /folder/{folderId}
    if (target.length() <= 8 || target.substr(0, 8) != "/folder/")
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid folder endpoint");
    }

    std::string folder_id_str = target.substr(8); // Remove "/folder/"
    if (folder_id_str.empty())
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Folder ID is required");
    }

    int folder_id;
    try
    {
      folder_id = std::stoi(folder_id_str);
    }
    catch (...)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid folder ID format");
    }

    auto db = Database::create();
    if (!db)
    {
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Database connection failed");
    }

    auto folder_result = db->getFolderById(folder_id);
    if (!folder_result.success() || !folder_result.value.has_value())
    {
      return create_error_response(boost::beast::http::status::not_found, req.version(),
                                   "Folder not found");
    }

    // Helper function to recursively delete all files in folder and subfolders from storage
    std::function<bool(int)> deleteFilesFromStorageRecursively = [&](int folderId) -> bool
    {
      // Get all files in this folder
      auto files_result = db->getFilesByFolder(folderId);
      if (files_result.success() && files_result.value.has_value())
      {
        for (const auto &file_record : files_result.value.value())
        {
          if (!FileStorage::deleteFile(file_record.storageId))
          {
            std::cerr << "Warning: Failed to delete file " << file_record.storageId << " from storage" << std::endl;
            // Continue with other files
          }
        }
      }

      // Get all subfolders and recursively delete their files
      auto subfolders_result = db->getFoldersByParent(folderId);
      if (subfolders_result.success() && subfolders_result.value.has_value())
      {
        for (const auto &subfolder : subfolders_result.value.value())
        {
          if (!deleteFilesFromStorageRecursively(subfolder.id))
          {
            return false;
          }
        }
      }

      return true;
    };

    // Delete all files from storage first (before database cascade deletion)
    if (!deleteFilesFromStorageRecursively(folder_id))
    {
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Failed to delete some files from storage");
    }

    // Then delete the folder from database (this will cascade to all subfolders and files)
    auto delete_result = db->deleteFolder(folder_id);
    if (!delete_result.success() || !delete_result.value.has_value() || !delete_result.value.value())
    {
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Failed to delete folder from database");
    }

    return create_success_response(boost::beast::http::status::ok, req.version(),
                                   "application/json", R"({"message":"Folder deleted successfully"})");
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_patch_file_move(const boost::beast::http::request<boost::beast::http::string_body> &req)
  {
    std::string target = std::string(req.target());

    // Extract file ID from /files/{fileId}/move
    if (target.length() <= 12 || target.substr(0, 7) != "/files/" || target.substr(target.length() - 5) != "/move")
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid file move endpoint");
    }

    std::string file_id_str = target.substr(7, target.length() - 12); // Remove "/files/" prefix and "/move"
    if (file_id_str.empty())
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "File ID is required");
    }

    int file_id;
    try
    {
      file_id = std::stoi(file_id_str);
    }
    catch (...)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid file ID format");
    }

    auto content_type_it = req.find(boost::beast::http::field::content_type);
    if (content_type_it == req.end() ||
        content_type_it->value().find("application/json") == std::string::npos)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Content-Type must be application/json");
    }

    std::string body = req.body();
    int folder_id;

    size_t folder_pos = body.find("\"folder_id\"");
    if (folder_pos == std::string::npos)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Missing 'folder_id' field in JSON");
    }

    size_t colon_pos = body.find(":", folder_pos);
    size_t number_start = body.find_first_of("0123456789", colon_pos + 1);
    size_t number_end = body.find_first_not_of("0123456789", number_start);

    if (colon_pos == std::string::npos || number_start == std::string::npos)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid 'folder_id' field in JSON");
    }

    try
    {
      folder_id = std::stoi(body.substr(number_start, number_end - number_start));
    }
    catch (...)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Failed to parse folder_id. Expected integer value");
    }

    auto db = Database::create();
    if (!db)
    {
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Database connection failed");
    }

    auto file_result = db->getFileById(file_id);
    if (!file_result.success() || !file_result.value.has_value())
    {
      return create_error_response(boost::beast::http::status::not_found, req.version(),
                                   "File not found");
    }

    auto folder_result = db->getFolderById(folder_id);
    if (!folder_result.success() || !folder_result.value.has_value())
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Target folder not found");
    }

    auto move_result = db->moveFile(file_id, folder_id);
    if (!move_result.success() || !move_result.value.has_value() || !move_result.value.value())
    {
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   move_result.errorMessage.empty() ? "Failed to move file" : move_result.errorMessage);
    }

    return create_success_response(boost::beast::http::status::ok, req.version(),
                                   "application/json", R"({"message":"File moved successfully"})");
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_delete_file_tag(const boost::beast::http::request<boost::beast::http::string_body> &req)
  {
    std::string target = std::string(req.target());
    
    // Extract file ID and tag ID from /files/{fileId}/tags/{tagId}
    if (target.length() <= 13 || target.substr(0, 7) != "/files/")
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid file tag endpoint");
    }

    // Find the position of "/tags/"
    size_t tags_pos = target.find("/tags/");
    if (tags_pos == std::string::npos || tags_pos <= 7)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid file tag endpoint format");
    }

    // Extract file ID (between "/files/" and "/tags/")
    std::string file_id_str = target.substr(7, tags_pos - 7);
    
    // Extract tag ID (after "/tags/")
    std::string tag_id_str = target.substr(tags_pos + 6);
    
    if (file_id_str.empty() || tag_id_str.empty())
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Both file ID and tag ID are required");
    }

    int file_id, tag_id;
    try
    {
      file_id = std::stoi(file_id_str);
      tag_id = std::stoi(tag_id_str);
    }
    catch (...)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid file ID or tag ID format");
    }

    auto db = Database::create();
    if (!db)
    {
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Database connection failed");
    }

    auto file_result = db->getFileById(file_id);
    if (!file_result.success() || !file_result.value.has_value())
    {
      return create_error_response(boost::beast::http::status::not_found, req.version(),
                                   "File not found");
    }

    auto tag_result = db->getTagById(tag_id);
    if (!tag_result.success() || !tag_result.value.has_value())
    {
      return create_error_response(boost::beast::http::status::not_found, req.version(),
                                   "Tag not found");
    }

    auto remove_result = db->removeFileTag(file_id, tag_id);
    if (!remove_result.success() || !remove_result.value.has_value() || !remove_result.value.value())
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   remove_result.errorMessage.empty() ? "Failed to remove tag from file" : remove_result.errorMessage);
    }

    return create_success_response(boost::beast::http::status::ok, req.version(),
                                   "application/json", R"({"message":"Tag removed from file successfully"})");
  }

  boost::beast::http::response<boost::beast::http::string_body>
  handle_delete_file_metadata(const boost::beast::http::request<boost::beast::http::string_body> &req)
  {
    std::string target = std::string(req.target());
    
    // Extract file ID and metadata key from /files/{fileId}/metadata/{key}
    if (target.length() <= 17 || target.substr(0, 7) != "/files/")
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid file metadata endpoint");
    }

    // Find the position of "/metadata/"
    size_t metadata_pos = target.find("/metadata/");
    if (metadata_pos == std::string::npos || metadata_pos <= 7)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid file metadata endpoint format");
    }

    // Extract file ID (between "/files/" and "/metadata/")
    std::string file_id_str = target.substr(7, metadata_pos - 7);
    std::string metadata_key = target.substr(metadata_pos + 10);
    
    if (file_id_str.empty() || metadata_key.empty())
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Both file ID and metadata key are required");
    }

    int file_id;
    try
    {
      file_id = std::stoi(file_id_str);
    }
    catch (...)
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   "Invalid file ID format");
    }

    auto db = Database::create();
    if (!db)
    {
      return create_error_response(boost::beast::http::status::internal_server_error, req.version(),
                                   "Database connection failed");
    }

    auto file_result = db->getFileById(file_id);
    if (!file_result.success() || !file_result.value.has_value())
    {
      return create_error_response(boost::beast::http::status::not_found, req.version(),
                                   "File not found");
    }

    auto remove_result = db->removeFileMetadata(file_id, metadata_key);
    if (!remove_result.success() || !remove_result.value.has_value() || !remove_result.value.value())
    {
      return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                   remove_result.errorMessage.empty() ? "Failed to remove metadata from file" : remove_result.errorMessage);
    }

    return create_success_response(boost::beast::http::status::ok, req.version(),
                                   "application/json", R"({"message":"Metadata removed from file successfully"})");
  }

  boost::beast::http::message_generator handle_request(boost::beast::http::request<boost::beast::http::string_body> &&req)
  {
    // Handle OPTIONS requests for CORS preflight
    if (req.method() == boost::beast::http::verb::options)
      return handle_options(req.version());

    // GET /health
    if (req.method() == boost::beast::http::verb::get && req.target() == "/health")
      return handle_health(req.version());

    // GET /
    if (req.method() == boost::beast::http::verb::get && req.target() == "/")
      return handle_root(req.version());

    // GET /folder, /folder/, or /folder/{id}
    if (req.method() == boost::beast::http::verb::get &&
        (req.target() == "/folder" || req.target() == "/folder/" ||
         (req.target().length() > 8 && std::string(req.target()).substr(0, 8) == "/folder/")))
      return handle_get_folder(req);

    // POST /folder
    if (req.method() == boost::beast::http::verb::post && req.target() == "/folder")
      return handle_post_folder(req);

    // DELETE /folder/{folderId}
    if (req.method() == boost::beast::http::verb::delete_ &&
        req.target().length() > 8 && std::string(req.target()).substr(0, 8) == "/folder/")
      return handle_delete_folder(req);

    // PATCH /folder/{folderId}/move

    // TODO: maybe rename this to /files?
    // POST /upload
    if (req.method() == boost::beast::http::verb::post && req.target() == "/upload")
      return handle_post_upload(req);

    // DELETE /files/{fileId}
    if (req.method() == boost::beast::http::verb::delete_ &&
        req.target().length() > 7 && std::string(req.target()).substr(0, 7) == "/files/")
      return handle_delete_file(req);

    // PATCH /files/{fileId}/move
    if (req.method() == boost::beast::http::verb::patch &&
        req.target().length() > 12 && std::string(req.target()).substr(0, 7) == "/files/" &&
        std::string(req.target()).substr(req.target().length() - 5) == "/move")
      return handle_patch_file_move(req);

    // GET /download/{id}
    if (req.method() == boost::beast::http::verb::get &&
        req.target().length() > 10 && std::string(req.target()).substr(0, 10) == "/download/")
      return handle_get_download(req);

    // GET /tags - gets all tags
    if (req.method() == boost::beast::http::verb::get && req.target() == "/tags")
      return handle_get_tags(req);

    // POST /tags - create a new tag
    if (req.method() == boost::beast::http::verb::post && req.target() == "/tags")
      return handle_post_tags(req);

    // POST /files/{fileId}/tags - add tag to file
    if (req.method() == boost::beast::http::verb::post &&
        req.target().length() > 7 && std::string(req.target()).substr(0, 7) == "/files/" &&
        std::string(req.target()).find("/tags") != std::string::npos)
      return handle_post_file_tags(req);

    // DELETE /files/{fileId}/tags/{tagId} - remove tag from file
    if (req.method() == boost::beast::http::verb::delete_ &&
        req.target().length() > 13 && std::string(req.target()).substr(0, 7) == "/files/" &&
        std::string(req.target()).find("/tags/") != std::string::npos)
      return handle_delete_file_tag(req);

    // POST /files/{fileId}/metadata - add metadata to file
    if (req.method() == boost::beast::http::verb::post &&
        req.target().length() > 7 && std::string(req.target()).substr(0, 7) == "/files/" &&
        std::string(req.target()).find("/metadata") != std::string::npos)
      return handle_post_file_metadata(req);

    // DELETE /files/{fileId}/metadata/{key} - remove metadata from file
    if (req.method() == boost::beast::http::verb::delete_ &&
        req.target().length() > 17 && std::string(req.target()).substr(0, 7) == "/files/" &&
        std::string(req.target()).find("/metadata/") != std::string::npos)
      return handle_delete_file_metadata(req);

    // 404 Not Found
    return create_success_response(boost::beast::http::status::not_found, req.version(),
                                   "text/plain", "Not found");
  }
}