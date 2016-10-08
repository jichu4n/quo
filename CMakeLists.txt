# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                             #
#    Copyright (C) 2016 Chuan Ji <jichu4n@gmail.com>                          #
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
set(CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/deps")

# Configure protocol buffers.
include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
link_libraries(${Protobuf_LIBRARIES})

# AST.
include_directories("${PROJECT_SOURCE_DIR}/ast")
add_subdirectory("ast")

