#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <string>

namespace bytebucket
{
  template <typename T>
  void addCorsHeaders(boost::beast::http::response<T> &res);

  // Helper functions for creating responses
  boost::beast::http::response<boost::beast::http::string_body>
  create_error_response(boost::beast::http::status status, unsigned version, const std::string &error_message);

  boost::beast::http::response<boost::beast::http::string_body>
  create_success_response(boost::beast::http::status status, unsigned version, const std::string &content_type, const std::string &body);

  // Endpoint handlers
  boost::beast::http::response<boost::beast::http::string_body>
  handle_options(unsigned version);

  boost::beast::http::response<boost::beast::http::string_body>
  handle_health(unsigned version);

  boost::beast::http::response<boost::beast::http::string_body>
  handle_root(unsigned version);

  boost::beast::http::response<boost::beast::http::string_body> handle_get_folder(const boost::beast::http::request<boost::beast::http::string_body> &req);

  boost::beast::http::response<boost::beast::http::string_body>
  handle_post_folder(const boost::beast::http::request<boost::beast::http::string_body> &req);

  boost::beast::http::response<boost::beast::http::string_body>
  handle_post_upload(const boost::beast::http::request<boost::beast::http::string_body> &req);

  boost::beast::http::message_generator
  handle_get_download(const boost::beast::http::request<boost::beast::http::string_body> &req);

  boost::beast::http::response<boost::beast::http::string_body>
  handle_get_tags(const boost::beast::http::request<boost::beast::http::string_body> &req);

  boost::beast::http::response<boost::beast::http::string_body>
  handle_post_tags(const boost::beast::http::request<boost::beast::http::string_body> &req);

  boost::beast::http::response<boost::beast::http::string_body>
  handle_post_file_tags(const boost::beast::http::request<boost::beast::http::string_body> &req);

  // Main request handler
  boost::beast::http::message_generator handle_request(boost::beast::http::request<boost::beast::http::string_body> &&req);
}
