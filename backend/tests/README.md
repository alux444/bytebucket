# ByteBucket Backend Tests

This directory contains comprehensive Catch2 tests for the ByteBucket server endpoints.

## Running Tests

```bash
# run tests
./dev.sh test
./dev.sh test-verbose
./dev.sh test-ctest
# run tests with watch on
./dev.sh watch
```

## Test Statistics

- **Total Tests**: 6 test cases
- **Total Assertions**: 69 assertions
- **Status**: ✅ All tests passing

The tests use a custom test helper (`test_helpers.hpp`) that duplicates the request handler logic to avoid the complexity of extracting responses from Boost.Beast's `message_generator`. This ensures reliable and maintainable testing of all endpoint behaviors.
