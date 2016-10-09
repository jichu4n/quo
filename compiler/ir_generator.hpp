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

#ifndef IR_GENERATOR_HPP_
#define IR_GENERATOR_HPP_

#include <memory>
#include <string>
#include <unordered_map>
#include "llvm/LinkAllIR.h"
#include "ast/ast.pb.h"

namespace quo {

// Implements AST to IR generation.
class IRGenerator {
 public:
  IRGenerator();

  ::std::unique_ptr<::llvm::Module> ProcessModule(const ModuleDef& module_def);

 protected:
  void ProcessModuleMember(
      ::llvm::Module* module, const ModuleDef::Member& member);
  void ProcessModuleFuncDef(::llvm::Module* module, const FuncDef& func_def);

  ::llvm::Type* LookupType(const TypeSpec& type_spec);

 private:
  void SetupBuiltinTypes();

  ::llvm::LLVMContext ctx_;
  struct {
    ::llvm::StructType* object_ty;
    ::llvm::StructType* int32_ty;
    ::llvm::StructType* string_ty;
  } builtin_types_;
  ::std::unordered_map<::std::string, ::llvm::Type*> builtin_types_map_;
};

}  // namespace quo

#endif  // IR_GENERATOR_HPP_
