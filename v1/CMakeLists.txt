# Same CMake version requirement as LLVM 3.9.
cmake_minimum_required(VERSION 3.4.3)

# Project info.
project(quo CXX)

# Configure protocol buffer compiler and libs.
include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
link_libraries(${Protobuf_LIBRARIES})

# Sub-directories.
add_subdirectory("ast")
add_subdirectory("parser")

