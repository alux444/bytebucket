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
#include "request_handler.hpp"
#include "database.hpp"

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

      // Store keep_alive status before moving the request
      bool keep_alive = req.keep_alive();

      boost::beast::http::message_generator response = bytebucket::handle_request(std::move(req));
      boost::beast::write(socket, response);

      if (!keep_alive)
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
    std::cout << "Initialising db..." << std::endl;
    auto db = bytebucket::Database::create();
    if (!db)
    {
      std::cerr << "Failed to initialise db" << std::endl;
      return EXIT_FAILURE;
    }
    std::cout << "Initialised db!" << std::endl;

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
