#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// This function produces an HTTP response for the given request.
http::message_generator handle_request(http::request<http::string_body> &&req)
{
  // Respond to GET /health
  if (req.method() == http::verb::get && req.target() == "/health")
  {
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ByteBucket-Server");
    res.set(http::field::content_type, "application/json");
    res.body() = R"({\"status\":\"ok\"})";
    res.prepare_payload();
    return res;
  }
  // Default: 404 Not Found
  http::response<http::string_body> res{http::status::not_found, req.version()};
  res.set(http::field::server, "ByteBucket-Server");
  res.set(http::field::content_type, "text/plain");
  res.body() = "Not found";
  res.prepare_payload();
  return res;
}

void do_session(tcp::socket socket)
{
  beast::flat_buffer buffer;
  try
  {
    for (;;)
    {
      http::request<http::string_body> req;
      http::read(socket, buffer, req);
      auto response = handle_request(std::move(req));
      beast::write(socket, response);
      if (response.keep_alive() == false)
        break;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Session error: " << e.what() << std::endl;
  }
}

int main(int argc, char *argv[])
{
  try
  {
    auto const address = net::ip::make_address("0.0.0.0");
    unsigned short port = 8080;
    net::io_context ioc{1};
    tcp::acceptor acceptor{ioc, {address, port}};
    std::cout << "Server started on http://0.0.0.0:8080\n";
    for (;;)
    {
      tcp::socket socket{ioc};
      acceptor.accept(socket);
      std::thread([socket = std::move(socket)]() mutable
                  { do_session(std::move(socket)); })
          .detach();
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
