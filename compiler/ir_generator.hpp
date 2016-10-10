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
#include <llvm/IR/IRBuilder.h>
#include <llvm/LinkAllIR.h>
#include "ast/ast.pb.h"

namespace quo {

// Implements AST to IR generation.
class IRGenerator {
 public:
  IRGenerator();

  ::std::unique_ptr<::llvm::Module> ProcessModule(const ModuleDef& module_def);

 protected:
  struct State;
  struct ExprResult {
    ::llvm::Value* value;
    ::llvm::Value* address;
  };

  void ProcessModuleMember(
      State* state, const ModuleDef::Member& member);
  void ProcessModuleFnDef(State* state, const FnDef& fn_def);
  void ProcessBlock(State* state, const Block& block);
  void ProcessRetStmt(State* state, const RetStmt& stmt);
  ExprResult ProcessExpr(State* state, const Expr& expr);
  ExprResult ProcessConstantExpr(State* state, const ConstantExpr& expr);
  ExprResult ProcessVarExpr(State* state, const VarExpr& expr);
  ExprResult ProcessBinaryOpExpr(State* state, const BinaryOpExpr& expr);

  ::llvm::Type* LookupType(const TypeSpec& type_spec);
  ::llvm::Value* CreateInt32Value(State* state, ::llvm::Value* raw_int32_value);

 private:
  void SetupBuiltinTypes();

  ::llvm::LLVMContext ctx_;
  struct {
    ::llvm::StructType* object_ty;
    ::llvm::StructType* int32_ty;
    ::llvm::StructType* bool_ty;
    ::llvm::StructType* string_ty;
  } builtin_types_;
  ::std::unordered_map<::std::string, ::llvm::Type*> builtin_types_map_;
};

}  // namespace quo

#endif  // IR_GENERATOR_HPP_
