#pragma once

#include <boost/beast/http.hpp>
#include "request_handler.hpp"
#include "multipart_parser.hpp"
#include "file_storage.hpp"
#include <sstream>

namespace bytebucket
{
  namespace test
  {
    // Helper function to create error responses (matching request_handler)
    inline boost::beast::http::response<boost::beast::http::string_body>
    create_error_response(boost::beast::http::status status, unsigned version, const std::string &error_message)
    {
      boost::beast::http::response<boost::beast::http::string_body> res{status, version};
      res.set(boost::beast::http::field::server, "ByteBucket-Server");
      res.set(boost::beast::http::field::content_type, "application/json");
      res.body() = R"({"error":")" + error_message + R"("})";
      res.prepare_payload();
      return res;
    }

    // Helper function to create success responses (matching request_handler)
    inline boost::beast::http::response<boost::beast::http::string_body>
    create_success_response(boost::beast::http::status status, unsigned version, const std::string &content_type, const std::string &body)
    {
      boost::beast::http::response<boost::beast::http::string_body> res{status, version};
      res.set(boost::beast::http::field::server, "ByteBucket-Server");
      res.set(boost::beast::http::field::content_type, content_type);
      res.body() = body;
      res.prepare_payload();
      return res;
    }

    // Health check endpoint handler (test version)
    inline boost::beast::http::response<boost::beast::http::string_body>
    handle_health(unsigned version)
    {
      return create_success_response(boost::beast::http::status::ok, version, "application/json", R"({"status":"ok"})");
    }

    // Root endpoint handler (test version)
    inline boost::beast::http::response<boost::beast::http::string_body>
    handle_root(unsigned version)
    {
      return create_success_response(boost::beast::http::status::ok, version, "text/plain", "ByteBucket");
    }

    // Folder creation endpoint handler (test version with mock database)
    inline boost::beast::http::response<boost::beast::http::string_body>
    handle_post_folder(const boost::beast::http::request<boost::beast::http::string_body> &req)
    {
      // Validate Content-Type
      auto content_type_it = req.find(boost::beast::http::field::content_type);
      if (content_type_it == req.end() ||
          content_type_it->value().find("application/json") == std::string::npos)
      {
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "Content-Type must be application/json");
      }

      std::string body = req.body();
      std::string folder_name;
      std::optional<int> parent_id;

      // Parse folder name
      size_t name_pos = body.find("\"name\"");
      if (name_pos == std::string::npos)
      {
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "Missing 'name' field in JSON");
      }

      size_t colon_pos = body.find(":", name_pos);
      size_t quote_start = body.find("\"", colon_pos);
      size_t quote_end = body.find("\"", quote_start + 1);
      if (colon_pos == std::string::npos || quote_start == std::string::npos || quote_end == std::string::npos)
      {
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "Invalid 'name' field in JSON");
      }

      folder_name = body.substr(quote_start + 1, quote_end - quote_start - 1);
      if (folder_name.empty())
      {
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "Folder name can't be empty");
      }

      // Parse optional parent_id
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

      // Mock database behavior for testing - use thread_local counter for predictable IDs
      thread_local int folder_counter = 1;
      int folder_id = folder_counter++;

      // Simulate database validation - parent_id must exist if provided
      if (parent_id.has_value() && parent_id.value() >= folder_counter)
      {
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "Failed to create folder");
      }

      // Build response JSON (matching real implementation)
      std::ostringstream response_json;
      response_json << R"({"id":)" << folder_id
                    << R"(,"name":")" << folder_name << R"(")";

      if (parent_id.has_value())
        response_json << R"(,"parent_id":)" << parent_id.value();
      else
        response_json << R"(,"parent_id":null)";

      response_json << "}";

      return create_success_response(boost::beast::http::status::created, req.version(),
                                     "application/json", response_json.str());
    }

    // File upload endpoint handler (test version with mock file storage)
    inline boost::beast::http::response<boost::beast::http::string_body>
    handle_post_upload(const boost::beast::http::request<boost::beast::http::string_body> &req)
    {
      // Validate Content-Type
      auto content_type_it = req.find(boost::beast::http::field::content_type);
      if (content_type_it == req.end())
      {
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "Content-Type header is required");
      }

      std::string content_type = std::string(content_type_it->value());
      if (content_type.find("multipart/form-data") == std::string::npos)
      {
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "Content-Type should be multipart/form-data");
      }

      // Extract boundary
      std::string boundary = MultipartParser::extractBoundary(content_type);
      if (boundary.empty())
      {
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "Invalid boundary in Content-Type");
      }

      // Parse multipart data
      auto multipart_data = MultipartParser::parse(req.body(), boundary);
      if (!multipart_data.has_value())
      {
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "Failed to parse multipart data");
      }

      if (multipart_data->files.empty())
      {
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "No files found in request");
      }

      // Mock file storage for testing - return predictable file IDs
      std::ostringstream response_json;
      response_json << R"({"files":[)";
      bool first_file = true;

      for (const auto &file : multipart_data->files)
      {
        // Generate predictable test file ID (matching existing tests)
        std::string file_id = "test_" + file.filename + "_123";

        if (!first_file)
          response_json << ",";
        first_file = false;

        response_json << R"({"id":")" << file_id << R"(",)"
                      << R"("filename":")" << file.filename << R"(",)"
                      << R"("content_type":")" << file.content_type << R"(",)"
                      << R"("size":)" << file.content.size() << "}";
      }

      response_json << "]}";

      return create_success_response(boost::beast::http::status::ok, req.version(),
                                     "application/json", response_json.str());
    }

    // File download endpoint handler (test version)
    inline boost::beast::http::response<boost::beast::http::string_body>
    handle_get_download(const boost::beast::http::request<boost::beast::http::string_body> &req)
    {
      std::string target = std::string(req.target());
      std::string file_id = target.substr(10); // Remove "/download/" prefix

      if (file_id.empty())
      {
        return create_error_response(boost::beast::http::status::bad_request, req.version(),
                                     "File ID is required");
      }

      // Mock check for file existence (test files start with "test_")
      if (file_id.length() >= 5 && file_id.substr(0, 5) == "test_")
      {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, req.version()};
        res.set(boost::beast::http::field::server, "ByteBucket-Server");
        res.set(boost::beast::http::field::content_type, "text/plain");
        res.set(boost::beast::http::field::content_disposition, "attachment; filename=\"test_file_" + file_id + ".txt\"");
        res.body() = "Found file ID! " + file_id;
        res.prepare_payload();
        return res;
      }
      else
      {
        return create_error_response(boost::beast::http::status::not_found, req.version(),
                                     "File not found");
      }
    }

    // Main test request handler - now matches the structure of the real handler
    inline boost::beast::http::response<boost::beast::http::string_body>
    handle_request_direct(boost::beast::http::request<boost::beast::http::string_body> &&req)
    {
      using namespace boost::beast::http;

      // GET /health
      if (req.method() == verb::get && req.target() == "/health")
      {
        return handle_health(req.version());
      }

      // GET /
      if (req.method() == verb::get && req.target() == "/")
      {
        return handle_root(req.version());
      }

      // POST /folder
      if (req.method() == verb::post && req.target() == "/folder")
      {
        return handle_post_folder(req);
      }

      // POST /upload
      if (req.method() == verb::post && req.target() == "/upload")
      {
        return handle_post_upload(req);
      }

      // GET /download/{id}
      if (req.method() == verb::get && req.target().starts_with("/download/"))
      {
        return handle_get_download(req);
      }

      // 404 Not Found
      return create_success_response(status::not_found, req.version(),
                                     "text/plain", "Not found");
    }

  } // namespace test
} // namespace bytebucket