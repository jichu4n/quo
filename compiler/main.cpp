/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2016 Chuan Ji <jichu4n@gmail.com>                          *
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

#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include "compiler/exceptions.hpp"
#include "compiler/ir_generator.hpp"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"
#include "llvm/Support/raw_ostream.h"

using ::google::protobuf::io::FileInputStream;
using ::google::protobuf::io::IstreamInputStream;
using ::quo::Exception;
using ::quo::IRGenerator;
using ::quo::ModuleDef;
using ::std::cerr;
using ::std::cin;
using ::std::endl;
using ::std::unique_ptr;

int main(int argc, char* argv[]) {
  ::gflags::ParseCommandLineFlags(&argc, &argv, true);
  ::google::InitGoogleLogging(argv[0]);

  ModuleDef module_def;
  if (argc > 1) {
    int fd = open(argv[1], O_RDONLY);
    FileInputStream input_stream(fd);
    CHECK(::google::protobuf::TextFormat::Parse(&input_stream, &module_def));
    close(fd);
  } else {
    IstreamInputStream input_stream(&cin);
    CHECK(::google::protobuf::TextFormat::Parse(&input_stream, &module_def));
  }
  // LOG(INFO) << module_def.DebugString();

  IRGenerator ir_generator;
  unique_ptr<::llvm::Module> module;
  try {
    module = ir_generator.ProcessModule(module_def);
  } catch (const Exception& e) {
    cerr << e.what() << endl;
    return EXIT_FAILURE;
  }

  module->print(
      ::llvm::outs(),
      nullptr,  // AssemblyAnnotationWriter
      false,    // ShouldPreserveUseListOrder
      true);    // IsForDebug

  return EXIT_SUCCESS;
}
