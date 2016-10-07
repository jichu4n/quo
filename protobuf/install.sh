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

CPP_URL='https://github.com/google/protobuf/releases/download/v3.1.0/protobuf-cpp-3.1.0.tar.gz'

set -ex
cd "$(dirname $0)"

# C++ libs and protoc compiler.
CPP_ARCHIVE="$(mktemp --suffix=.tar.gz)"
curl -L -o "$CPP_ARCHIVE" "$CPP_URL"
CPP_ROOT="$PWD/cpp"
mkdir -p cpp_src
cd cpp_src
tar -xvz --strip-components=1 -f "$CPP_ARCHIVE"
rm "$CPP_ARCHIVE"
./configure --prefix="$CPP_ROOT"
make -j
make install
cd ..
mv "$CPP_ROOT/bin/protoc" ./
rm -r 'cpp_src' "$CPP_ROOT/bin"

