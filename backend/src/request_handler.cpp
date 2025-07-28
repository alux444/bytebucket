#include "request_handler.hpp"
#include <boost/beast/http.hpp>
#include <string>

namespace bytebucket
{

  boost::beast::http::message_generator handle_request(boost::beast::http::request<boost::beast::http::string_body> &&req)
  {
    // GET [health]
    if (req.method() == boost::beast::http::verb::get && req.target() == "/health")
    {
      boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, req.version()};
      res.set(boost::beast::http::field::server, "ByteBucket-Server");
      res.set(boost::beast::http::field::content_type, "application/json");
      res.body() = R"({"status":"ok"})";
      res.prepare_payload();
      return res;
    }

    // GET [/]
    if (req.method() == boost::beast::http::verb::get && req.target() == "/")
    {
      // TODO: adjust to serve frontend files
      boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, req.version()};
      res.set(boost::beast::http::field::server, "ByteBucket-Server");
      res.set(boost::beast::http::field::content_type, "text/plain");
      res.body() = "ByteBucket";
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
        res.set(boost::beast::http::field::server, "ByteBucket-Server");
        res.set(boost::beast::http::field::content_type, "application/json");
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
        res.set(boost::beast::http::field::server, "ByteBucket-Server");
        res.set(boost::beast::http::field::content_type, "application/json");
        res.body() = R"({"error":"Content-Type should be multipart/form-data"})";
        res.prepare_payload();
        return res;
      }
      // TODO: upload functionality
      boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, req.version()};
      res.set(boost::beast::http::field::server, "ByteBucket-Server");
      res.set(boost::beast::http::field::content_type, "application/octet-stream");
      res.set(boost::beast::http::field::content_disposition, "attachment; filename=\"uploaded_file\"");

      res.body() = req.body();
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
        res.set(boost::beast::http::field::server, "ByteBucket-Server");
        res.set(boost::beast::http::field::content_type, "application/json");
        res.body() = R"({"error":"File ID is required"})";
        res.prepare_payload();
        return res;
      }

      if (file_id == "test123")
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
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, req.version()};
        res.set(boost::beast::http::field::server, "ByteBucket-Server");
        res.set(boost::beast::http::field::content_type, "application/json");
        res.body() = R"({"error":"File not found"})";
        res.prepare_payload();
        return res;
      }
    }

    boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, req.version()};
    res.set(boost::beast::http::field::server, "ByteBucket-Server");
    res.set(boost::beast::http::field::content_type, "text/plain");
    res.body() = "Not found";
    res.prepare_payload();
    return res;
  }

}
