cmake_minimum_required(VERSION 3.15)
project(ByteBucket)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set CMake policies for Boost
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Find packages
find_package(PkgConfig REQUIRED)
find_package(Catch2 3 QUIET)

# For Boost 1.89+, just find Boost and thread component
# boost_system is header-only and not needed as a component
find_package(Boost REQUIRED COMPONENTS thread)
set(BOOST_LIBRARIES Boost::boost Boost::thread)

# If Catch2 3 is not found, try Catch2 2
if(NOT Catch2_FOUND)
    find_package(Catch2 2 REQUIRED)
endif()

# Find SQLite3
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Include directories
include_directories(include)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${SQLITE3_INCLUDE_DIRS})

# Add source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# Create main executable
add_executable(bytebucket src/main.cpp ${SOURCES})

# Link libraries for main executable
target_link_libraries(bytebucket 
    ${BOOST_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)

# Create test executable
if(Catch2_FOUND)
    file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
    add_executable(bytebucket_tests ${TEST_SOURCES} ${SOURCES})
    
    # Link libraries for test executable
    target_link_libraries(bytebucket_tests 
        ${BOOST_LIBRARIES}
        ${SQLITE3_LIBRARIES}
        Catch2::Catch2WithMain
    )
    
    # Enable testing
    enable_testing()
    add_test(NAME ByteBucketTests COMMAND bytebucket_tests)
endif()

# Compiler flags
target_compile_options(bytebucket PRIVATE -Wall -Wextra)
if(TARGET bytebucket_tests)
    target_compile_options(bytebucket_tests PRIVATE -Wall -Wextra)
endif()