#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <string>

namespace bytebucket
{
  void addCorsHeaders(boost::beast::http::response<boost::beast::http::string_body> &res);
  boost::beast::http::message_generator handle_request(boost::beast::http::request<boost::beast::http::string_body> &&req);
}
