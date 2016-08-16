#!/bin/bash
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                             #
#    Copyright (C) 2015 Chuan Ji <jichuan89@gmail.com>                        #
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

LLVM_DIR_NAME='clang+llvm-3.7.1-x86_64-linux-gnu-ubuntu-14.04'
LLVM_URL="http://llvm.org/releases/3.7.1/${LLVM_DIR_NAME}.tar.xz"
LLVM_DEST_DIR=/opt
LLVMLITE_VERSION='0.12.1'

pip2 install enum34
curl "$LLVM_URL" | \
  sudo tar xv --xz -C "$LLVM_DEST_DIR"
for pip in pip2 pip3; do
  sudo -H env \
    LLVM_CONFIG="$LLVM_DEST_DIR/$LLVM_DIR_NAME/bin/llvm-config" \
    CXX="$LLVM_DEST_DIR/$LLVM_DIR_NAME/bin/clang++" \
    CXX_FLTO_FLAGS= \
    LD_FLTO_FLAGS= \
    $pip install "llvmlite==${LLVMLITE_VERSION}"
done

