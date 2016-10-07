#!/bin/bash
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

PROTOBUF_CPP_URL='https://github.com/google/protobuf/releases/download/v3.1.0/protobuf-cpp-3.1.0.tar.gz'
CMAKE_URL='https://cmake.org/files/v3.6/cmake-3.6.2.tar.gz'
LLVM_URL='http://llvm.org/releases/3.9.0/llvm-3.9.0.src.tar.xz'

set -ex
cd "$(dirname $0)"
prefix="$PWD/deps"

# Protocol buffer compiler and C++ libs.
function install_protobuf() {
  protobuf_cpp_archive="$prefix/src/protobuf-cpp.tar.gz"
  protobuf_cpp_src="$prefix/src/protobuf"
  if [ ! -e "$protobuf_cpp_archive" ]; then
    curl -L -o "$protobuf_cpp_archive" "$PROTOBUF_CPP_URL"
  fi
  if [ -e "$protobuf_cpp_src/configure" ]; then
    cd "$protobuf_cpp_src"
  else
    mkdir -p "$protobuf_cpp_src"
    cd "$protobuf_cpp_src"
    tar -xvz --strip-components=1 -f "$protobuf_cpp_archive"
  fi
  ./configure --prefix="$prefix"
  make -j4
  make install
  cd "$prefix"
}

# CMake (required by LLVM).
function install_cmake() {
  cd "$prefix"
  cmake_archive="$prefix/src/cmake.tar.gz"
  cmake_src="$prefix/src/cmake"
  if [ ! -e "$cmake_archive" ]; then
    curl -L -o "$cmake_archive" "$CMAKE_URL"
  fi
  if [ -e "$cmake_src/bootstrap" ]; then
    cd "$cmake_src"
  else
    mkdir -p "$cmake_src"
    cd "$cmake_src"
    tar -xvz --strip-components=1 -f "$cmake_archive"
  fi
  ./bootstrap --prefix="$prefix" --parallel=4
  make -j4
  make install
}

# LLVM libs.
function install_llvm() {
  cd "$prefix"
  llvm_archive="$prefix/src/llvm.tar.xz"
  llvm_src="$prefix/src/llvm"
  llvm_build="$prefix/src/llvm_build"
  if [ ! -e "$llvm_archive" ]; then
    curl -L -o "$llvm_archive" "$LLVM_URL"
  fi
  if [ ! -e "$llvm_src/CMakeLists.txt" ]; then
    mkdir -p "$llvm_src"
    cd "$llvm_src"
    tar -xvJ --strip-components=1 -f "$llvm_archive"
    cd "$prefix"
  fi
  mkdir -p "$llvm_build"
  cd "$llvm_build"
  "$prefix/bin/cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    "$llvm_src"
  make -j4
  make install
}

time install_protobuf
time install_cmake
time install_llvm

