# Same CMake version requirement as LLVM.
cmake_minimum_required(VERSION 3.4.3)

# Project info.
project(quo CXX)

# Set global compilation flags.
include_directories("${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
set(CMAKE_POSITION_INDEPENDENT_CODE FALSE)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g")

# Enable tests.
enable_testing()

# Sub-directories.
add_subdirectory("runtime")
add_subdirectory("parser")

