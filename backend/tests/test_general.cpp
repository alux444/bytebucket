#include <catch2/catch_test_macros.hpp>
#include <boost/beast/http.hpp>
#include "request_handler.hpp"
#include "test_helpers.hpp"

TEST_CASE("General request handler tests", "[general]") {
    using namespace boost::beast::http;
    
    SECTION("Request handler preserves HTTP version in response") {
        // Test HTTP/1.0
        request<string_body> req10{verb::get, "/health", 10};
        req10.set(field::host, "localhost");
        
        auto response10 = bytebucket::test::handle_request_direct(std::move(req10));
        
        REQUIRE(response10.version() == 10);
        
        // Test HTTP/1.1
        request<string_body> req11{verb::get, "/health", 11};
        req11.set(field::host, "localhost");
        
        auto response11 = bytebucket::test::handle_request_direct(std::move(req11));
        
        REQUIRE(response11.version() == 11);
    }
    
    SECTION("All responses include ByteBucket-Server header") {
        std::vector<std::pair<std::string, verb>> test_cases = {
            {"/health", verb::get},
            {"/", verb::get},
            {"/upload", verb::post},
            {"/download/test123", verb::get},
            {"/unknown", verb::get}
        };
        
        for (const auto& [path, method] : test_cases) {
            request<string_body> req{method, path, 11};
            req.set(field::host, "localhost");
            
            if (method == verb::post) {
                req.body() = "test upload content";
                req.prepare_payload();
            }
            
            auto response = bytebucket::test::handle_request_direct(std::move(req));
            
            // Every response should have the server header
            REQUIRE(response[field::server] == "ByteBucket-Server");
        }
    }
    
    SECTION("Case sensitivity in paths") {
        // Test case sensitivity - our paths should be case sensitive
        std::vector<std::string> case_variants = {
            "/Health",
            "/HEALTH", 
            "/Upload",
            "/UPLOAD"
        };
        
        for (const auto& path : case_variants) {
            request<string_body> req{verb::get, path, 11};
            req.set(field::host, "localhost");
            
            auto response = bytebucket::test::handle_request_direct(std::move(req));
            
            // All uppercase/mixed case paths should return 404
            REQUIRE(response.result() == status::not_found);
        }
    }
}
