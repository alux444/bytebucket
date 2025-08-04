#include "request_handler.hpp"
#include "multipart_parser.hpp"
#include "file_storage.hpp"
#include "database.hpp"
#include <boost/beast/http.hpp>
#include <string>

namespace bytebucket
{
  const std::string SERVER_NAME{"ByteBucket-Server"};

  void addCorsHeaders(boost::beast::http::response<boost::beast::http::string_body> &res)
  {
    // TODO: specific frontend access control urls
    res.set(boost::beast::http::field::access_control_allow_origin, "*");
    res.set(boost::beast::http::field::access_control_allow_methods, "GET, POST, OPTIONS");
    res.set(boost::beast::http::field::access_control_allow_headers, "Content-Type");
  }

  boost::beast::http::message_generator handle_request(boost::beast::http::request<boost::beast::http::string_body> &&req)
  {
    // Handle OPTIONS requests for CORS preflight
    if (req.method() == boost::beast::http::verb::options)
    {
      boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, req.version()};
      res.set(boost::beast::http::field::server, SERVER_NAME);
      addCorsHeaders(res);
      res.prepare_payload();
      return res;
    }

    // GET [health]
    if (req.method() == boost::beast::http::verb::get && req.target() == "/health")
    {
      boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, req.version()};
      res.set(boost::beast::http::field::server, SERVER_NAME);
      res.set(boost::beast::http::field::content_type, "application/json");
      addCorsHeaders(res);
      res.body() = R"({"status":"ok"})";
      res.prepare_payload();
      return res;
    }

    // GET [/]
    if (req.method() == boost::beast::http::verb::get && req.target() == "/")
    {
      // TODO: adjust to serve frontend files
      boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, req.version()};
      res.set(boost::beast::http::field::server, SERVER_NAME);
      res.set(boost::beast::http::field::content_type, "text/plain");
      addCorsHeaders(res);
      res.body() = "ByteBucket";
      res.prepare_payload();
      return res;
    }

    // POST [/folder]
    if (req.method() == boost::beast::http::verb::post && req.target() == "/folder")
    {
      auto content_type_it = req.find(boost::beast::http::field::content_type);
      if (content_type_it == req.end() ||
          content_type_it->value().find("application/json") == std::string::npos)
      {
        boost::beast::http::response<boost::beast::http::string_body> res{
            boost::beast::http::status::bad_request,
            req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "application/json");
        addCorsHeaders(res);
        res.body() = R"({"error":"Content-Type must be application/json"})";
        res.prepare_payload();
        return res;
      }

      std::string body = req.body();
      std::string folder_name;
      std::optional<int> parent_id;

      size_t name_pos = body.find("\"name\"");
      if (name_pos == std::string::npos)
      {
        boost::beast::http::response<boost::beast::http::string_body> res{
            boost::beast::http::status::bad_request,
            req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "application/json");
        addCorsHeaders(res);
        res.body() = R"({"error":"Missing 'name' field in JSON"})";
        res.prepare_payload();
        return res;
      }

      size_t colon_pos = body.find(":", name_pos);
      size_t quote_start = body.find("\"", colon_pos);
      size_t quote_end = body.find("\"", quote_start + 1);
      if (colon_pos == std::string::npos || quote_start == std::string::npos || quote_end == std::string::npos)
      {
        boost::beast::http::response<boost::beast::http::string_body> res{
            boost::beast::http::status::bad_request,
            req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "application/json");
        addCorsHeaders(res);
        res.body() = R"({"error":"Invalid 'name' field in JSON"})";
        res.prepare_payload();
        return res;
      }

      folder_name = body.substr(quote_start + 1, quote_end - quote_start - 2);
      if (folder_name.empty())
      {
        boost::beast::http::response<boost::beast::http::string_body> res{
            boost::beast::http::status::bad_request,
            req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "application/json");
        addCorsHeaders(res);
        res.body() = R"({"error":"Folder name can't be empty"})";
        res.prepare_payload();
        return res;
      }

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
          boost::beast::http::response<boost::beast::http::string_body> res{
              boost::beast::http::status::bad_request,
              req.version()};
          res.set(boost::beast::http::field::server, SERVER_NAME);
          res.set(boost::beast::http::field::content_type, "application/json");
          addCorsHeaders(res);
          res.body() = R"({"error":"Failed to parse parent_id. Expected argument is integer with no quotes, otherwise omitted for no parent id. "})";
          res.prepare_payload();
          return res;
        }
      }

      auto db = Database::create();
      if (!db)
      {
        boost::beast::http::response<boost::beast::http::string_body> res{
            boost::beast::http::status::bad_request,
            req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "application/json");
        addCorsHeaders(res);
        res.body() = R"({"error":"Database connection failed"})";
        res.prepare_payload();
        return res;
      }

      auto folder_id = db->insertFolder(folder_name, parent_id);
      if (!folder_id.has_value())
      {
        boost::beast::http::response<boost::beast::http::string_body> res{
            boost::beast::http::status::bad_request,
            req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "application/json");
        addCorsHeaders(res);
        res.body() = R"({"error":"Failed to create folder"})";
        res.prepare_payload();
        return res;
      }

      std::ostringstream res_oss;
      res_oss << "{";
      res_oss << R"("id":)" << folder_id.value();
      res_oss << R"(,"name":")" << folder_name;
      res_oss << R"(")";

      if (parent_id.has_value())
        res_oss << R"(,"parent_id":")" << parent_id.value();
      else
        res_oss << R"(,"parent_id":null)";

      res_oss << "}";

      boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::created, req.version()};
      res.set(boost::beast::http::field::server, SERVER_NAME);
      res.set(boost::beast::http::field::content_type, "application/json");
      addCorsHeaders(res);
      res.body() = res_oss.str();
      res.prepare_payload();
      return res;
    }

    // POST [/upload]
    if (req.method() == boost::beast::http::verb::post && req.target() == "/upload")
    {
      // make sure it's a multipart/form-data
      auto content_type_it = req.find(boost::beast::http::field::content_type);
      if (content_type_it == req.end())
      {
        boost::beast::http::response<boost::beast::http::string_body> res{
            boost::beast::http::status::bad_request,
            req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "application/json");
        addCorsHeaders(res);
        res.body() = R"({"error":"Content-Type header is required"})";
        res.prepare_payload();
        return res;
      }

      std::string content_type = std::string(content_type_it->value());
      if (content_type.find("multipart/form-data") == std::string::npos)
      {
        boost::beast::http::response<boost::beast::http::string_body> res{
            boost::beast::http::status::bad_request,
            req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "application/json");
        addCorsHeaders(res);
        res.body() = R"({"error":"Content-Type should be multipart/form-data"})";
        res.prepare_payload();
        return res;
      }

      // get boundary
      std::string boundary = MultipartParser::extractBoundary(content_type);
      if (boundary.empty())
      {
        boost::beast::http::response<boost::beast::http::string_body> res{
            boost::beast::http::status::bad_request,
            req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "application/json");
        addCorsHeaders(res);
        res.body() = R"({"error":"Invalid boundary in Content-Type"})";
        res.prepare_payload();
        return res;
      }

      // get multipart data
      auto multipart_data = MultipartParser::parse(req.body(), boundary);
      if (!multipart_data.has_value())
      {
        boost::beast::http::response<boost::beast::http::string_body> res{
            boost::beast::http::status::bad_request,
            req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "application/json");
        addCorsHeaders(res);
        res.body() = R"({"error":"Failed to parse multipart data"})";
        res.prepare_payload();
        return res;
      }

      if (multipart_data->files.empty())
      {
        boost::beast::http::response<boost::beast::http::string_body> res{
            boost::beast::http::status::bad_request,
            req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "application/json");
        addCorsHeaders(res);
        res.body() = R"({"error":"No files found in request"})";
        res.prepare_payload();
        return res;
      }

      std::string response_body = R"({"files":[)";
      bool first_file = true;
      for (const auto &file : multipart_data->files)
      {
        auto file_id = FileStorage::saveFile(file.filename, file.content, file.content_type);
        if (!file_id.has_value())
        {
          boost::beast::http::response<boost::beast::http::string_body> res{
              boost::beast::http::status::bad_request,
              req.version()};
          res.set(boost::beast::http::field::server, SERVER_NAME);
          res.set(boost::beast::http::field::content_type, "application/json");
          addCorsHeaders(res);
          res.body() = R"({"error":"Failed to save file"})";
          res.prepare_payload();
          return res;
        }

        if (!first_file)
        {
          response_body += ",";
        }
        first_file = false;

        response_body += R"({"id":")" + file_id.value() + R"(",)";
        response_body += R"("filename":")" + file.filename + R"(",)";
        response_body += R"("content_type":")" + file.content_type + R"(",)";
        response_body += R"("size":")" + std::to_string(file.content.size()) + R"("})";
      }

      response_body += "]}";

      boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, req.version()};
      res.set(boost::beast::http::field::server, SERVER_NAME);
      res.set(boost::beast::http::field::content_type, "application/json");
      addCorsHeaders(res);
      res.body() = response_body;
      res.prepare_payload();
      return res;
    }

    // GET [/download/{id}]
    if (req.method() == boost::beast::http::verb::get && req.target().starts_with("/download/"))
    {
      // Extract file ID from URL path: /download/{id}
      std::string target = std::string(req.target());
      std::string file_id = target.substr(10); // Remove "/download/" prefix

      // TODO: look up file by ID from database/storage
      if (file_id.empty())
      {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::bad_request, req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "application/json");
        addCorsHeaders(res);
        res.body() = R"({"error":"File ID is required"})";
        res.prepare_payload();
        return res;
      }

      if (file_id == "test123")
      {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "text/plain");
        res.set(boost::beast::http::field::content_disposition, "attachment; filename=\"test_file_" + file_id + ".txt\"");
        addCorsHeaders(res);
        res.body() = "Found file ID! " + file_id;
        res.prepare_payload();
        return res;
      }
      else
      {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, req.version()};
        res.set(boost::beast::http::field::server, SERVER_NAME);
        res.set(boost::beast::http::field::content_type, "application/json");
        addCorsHeaders(res);
        res.body() = R"({"error":"File not found"})";
        res.prepare_payload();
        return res;
      }
    }

    boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, req.version()};
    res.set(boost::beast::http::field::server, SERVER_NAME);
    res.set(boost::beast::http::field::content_type, "text/plain");
    addCorsHeaders(res);
    res.body() = "Not found";
    res.prepare_payload();
    return res;
  }
}
