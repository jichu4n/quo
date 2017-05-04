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

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include "llvm/IR/IRBuilder.h"
#include "llvm/LinkAllIR.h"
#include "ast/ast.pb.h"

namespace quo {

class Scope;

// Implements AST to IR generation.
class IRGenerator {
 public:
  IRGenerator();

  ::std::unique_ptr<::llvm::Module> ProcessModule(const ModuleDef& module_def);

 protected:
  struct State;

  void ProcessModuleMember(
      State* state, const ModuleDef::Member& member);
  void ProcessModuleFnDef(State* state, const FnDef& fn_def);
  void ProcessBlock(
      State* state, const Block& block, bool manage_parent_scope = false);
  void ProcessRetStmt(State* state, const RetStmt& stmt);
  void ProcessCondStmt(State* state, const CondStmt& stmt);
  void ProcessVarDeclStmt(State* state, const VarDeclStmt& stmt);

  // Generates code to deallocate temps in a scope.
  void DestroyTemps(State* state);
  // Generates code to deallocate variables in all scopes up to and including
  // the function's root scope. Does NOT pop off the scopes.
  void DestroyFnScopes(State* state);

  ::llvm::LLVMContext ctx_;
};

}  // namespace quo

#endif  // IR_GENERATOR_HPP_
