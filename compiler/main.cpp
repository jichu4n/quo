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

#include "ast/ast.pb.h"
#include "compiler/ir_generator.hpp"
#include <memory>

using ::std::unique_ptr;

int main(int argc, char* argv[]) {
  ::quo::ast::ModuleDef module_def;

  ::quo::IRGenerator ir_generator;
  unique_ptr<::llvm::Module> module = ir_generator.run(module_def);

  module->dump();

  return 0;
}