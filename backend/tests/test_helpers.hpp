#pragma once

#include <boost/beast/http.hpp>
#include "request_handler.hpp"

namespace bytebucket
{
  namespace test
  {

    // Helper that works directly with the request handler
    // This bypasses the message_generator entirely for easier testing
    // implements same logic as request_handler - need to adjust everytime request_handler changes
    inline boost::beast::http::response<boost::beast::http::string_body>
    handle_request_direct(boost::beast::http::request<boost::beast::http::string_body> &&req)
    {
      using namespace boost::beast::http;

      // GET [health]
      if (req.method() == verb::get && req.target() == "/health")
      {
        response<string_body> res{status::ok, req.version()};
        res.set(field::server, "ByteBucket-Server");
        res.set(field::content_type, "application/json");
        res.body() = R"({"status":"ok"})";
        res.prepare_payload();
        return res;
      }

      // GET [/]
      if (req.method() == verb::get && req.target() == "/")
      {
        response<string_body> res{status::ok, req.version()};
        res.set(field::server, "ByteBucket-Server");
        res.set(field::content_type, "text/plain");
        res.body() = "ByteBucket";
        res.prepare_payload();
        return res;
      }

      // POST [/upload]
      if (req.method() == verb::post && req.target() == "/upload")
      {
        response<string_body> res{status::ok, req.version()};
        res.set(field::server, "ByteBucket-Server");
        res.set(field::content_type, "application/octet-stream");
        res.set(field::content_disposition, "attachment; filename=\"uploaded_file\"");
        res.body() = req.body();
        res.prepare_payload();
        return res;
      }

      // GET [/download/{id}]
      if (req.method() == verb::get && req.target().starts_with("/download/"))
      {
        std::string target = std::string(req.target());
        std::string file_id = target.substr(10); // Remove "/download/" prefix

        if (file_id.empty())
        {
          response<string_body> res{status::bad_request, req.version()};
          res.set(field::server, "ByteBucket-Server");
          res.set(field::content_type, "application/json");
          res.body() = R"({"error":"File ID is required"})";
          res.prepare_payload();
          return res;
        }

        if (file_id == "test123")
        {
          response<string_body> res{status::ok, req.version()};
          res.set(field::server, "ByteBucket-Server");
          res.set(field::content_type, "text/plain");
          res.set(field::content_disposition, "attachment; filename=\"test_file_" + file_id + ".txt\"");
          res.body() = "Found file ID! " + file_id;
          res.prepare_payload();
          return res;
        }
        else
        {
          response<string_body> res{status::not_found, req.version()};
          res.set(field::server, "ByteBucket-Server");
          res.set(field::content_type, "application/json");
          res.body() = R"({"error":"File not found"})";
          res.prepare_payload();
          return res;
        }
      }

      response<string_body> res{status::not_found, req.version()};
      res.set(field::server, "ByteBucket-Server");
      res.set(field::content_type, "text/plain");
      res.body() = "Not found";
      res.prepare_payload();
      return res;
    }

  } // namespace test
} // namespace bytebucket
