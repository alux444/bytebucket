cmake_minimum_required(VERSION 3.15)
project(bytebucket LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure vcpkg toolchain is used
if(DEFINED ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

# This line MUST be before find_package when using vcpkg
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}" CACHE STRING "Vcpkg toolchain")

# Enable CMake's Boost support
find_package(Boost REQUIRED COMPONENTS thread system)

# Collect sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Add executable
add_executable(bytebucket ${SOURCES})

# Include Boost headers
target_include_directories(bytebucket PRIVATE ${Boost_INCLUDE_DIRS})

# Link Boost libs
target_link_libraries(bytebucket PRIVATE ${Boost_LIBRARIES})
