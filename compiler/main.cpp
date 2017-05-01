/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2016-2017 Chuan Ji <ji@chu4n.com>                          *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *   http://www.apache.org/licenses/LICENSE-2.0                              *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <glog/logging.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <llvm/Support/raw_ostream.h>
#include "compiler/ir_generator.hpp"

using ::std::cin;
using ::std::unique_ptr;
using ::google::protobuf::io::IstreamInputStream;
using namespace ::quo;

int main(int argc, char* argv[]) {
  IstreamInputStream input_stream(&cin);
  ModuleDef module_def;
  CHECK(::google::protobuf::TextFormat::Parse(&input_stream, &module_def));
  // LOG(INFO) << module_def.DebugString();

  IRGenerator ir_generator;
  unique_ptr<::llvm::Module> module = ir_generator.ProcessModule(module_def);

  module->print(
      ::llvm::outs(),
      nullptr, // AssemblyAnnotationWriter
      false,  // ShouldPreserveUseListOrder
      true);  // IsForDebug

  return 0;
}

