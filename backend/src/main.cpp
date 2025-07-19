#include <boost/beast/core.hpp>      // Core Beast functionality
#include <boost/beast/http.hpp>      // HTTP protocol support
#include <boost/beast/version.hpp>   // Version information
#include <boost/asio/ip/tcp.hpp>     // TCP networking
#include <boost/asio/signal_set.hpp> // Signal handling
#include <boost/asio/strand.hpp>     // Thread synchronization
#include <cstdlib>                   // Standard library utilities
#include <iostream>                  // Input/output streams
#include <memory>                    // Smart pointers
#include <string>                    // String handling
#include <thread>                    // Multi-threading support

// Request handling
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

  boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, req.version()};
  res.set(boost::beast::http::field::server, "ByteBucket-Server");
  res.set(boost::beast::http::field::content_type, "text/plain");
  res.body() = "Not found";
  res.prepare_payload();
  return res;
}

// Handle a single client session - reads requests and sends responses
// Each session runs in its own thread to handle multiple concurrent clients
void do_session(boost::asio::ip::tcp::socket socket)
{
  boost::beast::flat_buffer buffer; // Buffer for reading HTTP data
  try
  {
    // Keep the connection alive for multiple requests (HTTP keep-alive)
    for (;;)
    {
      boost::beast::http::request<boost::beast::http::string_body> req;
      boost::beast::http::read(socket, buffer, req);

      boost::beast::http::message_generator response = handle_request(std::move(req));
      boost::beast::write(socket, response);
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
    auto const address = boost::asio::ip::make_address("0.0.0.0"); // Listen on all interfaces
    unsigned short port = 8080;

    boost::asio::io_context ioc{1};
    boost::asio::ip::tcp::acceptor acceptor{ioc, {address, port}};

    std::cout << "Server started on http://0.0.0.0:8080\n";
    std::cout << "Health check available at: http://0.0.0.0:8080/health\n";

    for (;;) // server loop
    {
      boost::asio::ip::tcp::socket socket{ioc};
      acceptor.accept(socket);

      std::thread([socket = std::move(socket)]() mutable
                  { do_session(std::move(socket)); })
          .detach(); // Detach thread so it runs independently
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
