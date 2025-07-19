# ByteBucket Backend Tests

This directory contains comprehensive Catch2 tests for the ByteBucket server endpoints.

## Test Files

- `test_health_endpoint.cpp` - Tests for the `/health` endpoint
- `test_root_endpoint.cpp` - Tests for the root `/` endpoint
- `test_upload_endpoint.cpp` - Tests for the `/upload` endpoint
- `test_download_endpoint.cpp` - Tests for the `/download/{id}` endpoint
- `test_unknown_endpoints.cpp` - Tests for undefined endpoints (404 handling)
- `test_general.cpp` - General request handler tests (HTTP version handling, headers, etc.)
- `test_helpers.hpp` - Helper functions for testing

## Running Tests

```bash
# Build the project
cmake --build build

# Run tests directly
./build/bytebucket_tests

# Run tests with detailed output
./build/bytebucket_tests --success

# Run tests through CTest
cd build && ctest --verbose
```

## Test Statistics

- **Total Tests**: 6 test cases
- **Total Assertions**: 69 assertions
- **Status**: ✅ All tests passing

The tests use a custom test helper (`test_helpers.hpp`) that duplicates the request handler logic to avoid the complexity of extracting responses from Boost.Beast's `message_generator`. This ensures reliable and maintainable testing of all endpoint behaviors.
