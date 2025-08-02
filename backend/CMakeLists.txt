cmake_minimum_required(VERSION 3.15)
project(bytebucket LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json for better IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ensure vcpkg toolchain is used
if(DEFINED ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

# This line MUST be before find_package when using vcpkg
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}" CACHE STRING "Vcpkg toolchain")

# Enable CMake's Boost support
find_package(Boost REQUIRED COMPONENTS thread system)

# Find Catch2 for testing
find_package(Catch2 3 REQUIRED)

# Find SQLite3
find_package(SQLite3 REQUIRED)

# Collect main application sources (excluding main.cpp for testing)
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Create a library for testable code (everything except main)
list(REMOVE_ITEM APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
if(APP_SOURCES)
    add_library(bytebucket_lib ${APP_SOURCES})
    target_include_directories(bytebucket_lib PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_include_directories(bytebucket_lib PRIVATE 
        ${Boost_INCLUDE_DIRS}
    )
    target_link_libraries(bytebucket_lib PRIVATE 
        ${Boost_LIBRARIES}
        SQLite::SQLite3
    )
endif()

# Add main executable
add_executable(bytebucket src/main.cpp)
target_include_directories(bytebucket PRIVATE 
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(bytebucket PRIVATE 
    ${Boost_LIBRARIES}
    SQLite::SQLite3
)
if(APP_SOURCES)
    target_link_libraries(bytebucket PRIVATE bytebucket_lib)
endif()

# Enable testing
enable_testing()

# Collect test sources
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)

if(TEST_SOURCES)
    # Add test executable
    add_executable(bytebucket_tests ${TEST_SOURCES})
    target_include_directories(bytebucket_tests PRIVATE 
        ${Boost_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(bytebucket_tests PRIVATE 
        ${Boost_LIBRARIES}
        SQLite::SQLite3
        Catch2::Catch2WithMain
    )
    if(APP_SOURCES)
        target_link_libraries(bytebucket_tests PRIVATE bytebucket_lib)
    endif()

    # Enable testing  
    enable_testing()
    
    # Add test executable to CTest manually
    add_test(NAME bytebucket_unit_tests COMMAND bytebucket_tests)
endif()