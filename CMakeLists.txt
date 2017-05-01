# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                             #
#    Copyright (C) 2016-2017 Chuan Ji <ji@chu4n.com>                          #
#                                                                             #
#    Licensed under the Apache License, Version 2.0 (the "License");          #
#    you may not use this file except in compliance with the License.         #
#    You may obtain a copy of the License at                                  #
#                                                                             #
#     http://www.apache.org/licenses/LICENSE-2.0                              #
#                                                                             #
#    Unless required by applicable law or agreed to in writing, software      #
#    distributed under the License is distributed on an "AS IS" BASIS,        #
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. #
#    See the License for the specific language governing permissions and      #
#    limitations under the License.                                           #
#                                                                             #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

# Same CMake version requirement as LLVM 3.9.
cmake_minimum_required(VERSION 3.4.3)

# Project info.
project(quo CXX)
set(quo_VERSION_MAJOR 0)
set(quo_VERSION_MINOR 1)
configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
)

# Use dependencies in the "deps" directory.
list(INSERT CMAKE_PREFIX_PATH 0 "${PROJECT_SOURCE_DIR}/deps")
# Use CMake libs in the "deps" directory.
list(INSERT CMAKE_MODULE_PATH 0 "${PROJECT_SOURCE_DIR}/deps/lib/cmake")

# Create __init__.py in build directory.
file(WRITE "${PROJECT_BINARY_DIR}/__init__.py")

# Set global compilation flags.
include_directories("${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
link_directories("${PROJECT_SOURCE_DIR}/deps/lib")
set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
set(BUILD_SHARED_LIBS OFF)
if(NOT APPLE)
  list(INSERT CMAKE_EXE_LINKER_FLAGS 0 "-static")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -g")

# Configure protocol buffer compiler and libs.
include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
# For some reason, this always yields shared libraries:
# link_libraries(${Protobuf_LIBRARIES})
find_library(protobuf_LIB "protobuf")
link_libraries(${protobuf_LIB})

# Configure Google logging library.
find_library(glog_LIB "glog")
link_libraries(${glog_LIB})

# Configure Google flags library.
find_library(gflags_LIB "gflags")
link_libraries(${gflags_LIB})

# Sub-directories.
add_subdirectory("ast")
add_subdirectory("runtime")
add_subdirectory("compiler")

