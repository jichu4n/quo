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

#ifndef IR_GENERATOR_HPP_
#define IR_GENERATOR_HPP_

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include "ast/ast.pb.h"
#include "compiler/builtins.hpp"
#include "compiler/expr_ir_generator.hpp"
#include "compiler/symbols.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/LinkAllIR.h"

namespace quo {

// Implements AST to IR generation.
class IRGenerator {
 public:
  IRGenerator();

  ::std::unique_ptr<::llvm::Module> ProcessModule(const ModuleDef& module_def);

 private:
  void ProcessModuleMember(const ModuleDef::Member& member);
  void ProcessClassDef(const ClassDef& class_def);
  void ProcessFnDef(const FnDef& fn_def, const ClassDef* parent_class_def);
  void ProcessBlock(const Block& block, bool is_fn_body = false);
  void ProcessRetStmt(const RetStmt& stmt);
  void ProcessBrkStmt(const BrkStmt& stmt);
  void ProcessCondStmt(const CondStmt& stmt);
  void ProcessCondLoopStmt(const CondLoopStmt& stmt);
  void ProcessVarDeclStmt(const VarDeclStmt& stmt);

  // Generates code to deallocate temps in a scope.
  void DestroyTemps();
  // Generates code to deallocate variables in a scope.
  void DestroyScope(Scope* scope = nullptr);
  // Generates code to deallocate variables in all scopes up to and including
  // the function's root scope. Does NOT pop off the scopes.
  void DestroyFnScopes();

  ::llvm::LLVMContext ctx_;
  // Current IR module being generated.
  ::llvm::Module* module_;
  // Current AST module being processed.
  const ModuleDef* module_def_;
  // IR type of the function currently being processed.
  ::llvm::FunctionType* fn_ty_;
  // IR function currently being processed.
  ::llvm::Function* fn_;
  // AST function currently being processed.
  const FnDef* fn_def_;
  // IR builder for the current function.
  ::llvm::IRBuilder<>* ir_builder_;
  // Stack of loop termination basic blocks.
  ::std::stack<::llvm::BasicBlock*> loop_end_bb_stack_;

  ::std::unique_ptr<Builtins> builtins_;
  ::std::unique_ptr<Symbols> symbols_;
  ::std::unique_ptr<ExprIRGenerator> expr_ir_generator_;
};

}  // namespace quo

#endif  // IR_GENERATOR_HPP_
