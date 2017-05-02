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
#include <vector>
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
  // The result of evaluating an expression. Only a subset of the fields will be
  // set for any given expression.
  struct ExprResult {
    // The underlying type of the expression, e.g. Int32
    TypeSpec type_spec;
    // The value of the expression (QObject).
    ::llvm::Value* value;
    // Address on the heap storing the value (QObject*). Only set if the value
    // is actually stored in memory (i.e. not an intermediate value).
    ::llvm::Value* address;
    // Address on the stack or heap of a reference pointing to this object
    // (QObject**). Only set if the expression resolves to a reference
    // (variable, instance member, or array element).
    ::llvm::Value* ref_address;
    // A function reference. Only set if the expression resolves to a function.
    const FnDef* fn_def;
  };

  void SetupBuiltinTypes();
  void SetupBuiltinGlobals(State* state);
  void SetupBuiltinFunctions(State* state);

  void ProcessModuleMember(
      State* state, const ModuleDef::Member& member);
  void ProcessModuleFnDef(State* state, const FnDef& fn_def);
  void ProcessBlock(
      State* state, const Block& block, bool manage_parent_scope = false);
  void ProcessRetStmt(State* state, const RetStmt& stmt);
  void ProcessCondStmt(State* state, const CondStmt& stmt);
  void ProcessVarDeclStmt(State* state, const VarDeclStmt& stmt);
  ExprResult ProcessExpr(State* state, const Expr& expr);
  ExprResult ProcessConstantExpr(State* state, const ConstantExpr& expr);
  ExprResult ProcessVarExpr(State* state, const VarExpr& expr);
  ExprResult ProcessCallExpr(State* state, const CallExpr& expr);
  ExprResult ProcessBinaryOpExpr(State* state, const BinaryOpExpr& expr);
  ExprResult ProcessAssignExpr(State* state, const AssignExpr& expr);

  ::llvm::Type* LookupType(const TypeSpec& type_spec);
  ::llvm::GlobalVariable* LookupDescriptor(
      State* state, const TypeSpec& type_spec);
  ::llvm::Value* CreateInt32Value(State* state, ::llvm::Value* raw_int32_value);
  ::llvm::Value* ExtractInt32Value(
      State* state, ::llvm::Value* wrapped_int32_value);
  ::llvm::Value* CreateBoolValue(State* state, ::llvm::Value* raw_bool_value);
  ::llvm::Value* ExtractBoolValue(
      State* state, ::llvm::Value* wrapped_bool_value);
  ::llvm::Value* CreateObject(State* state, const TypeSpec& type_spec);
  ::llvm::Value* CreateObject(
      State* state, const TypeSpec& type_spec, ::llvm::Value* init_value);
  ::llvm::Value* AssignObject(
      State* state,
      const TypeSpec& type_spec,
      ::llvm::Value* dest_address,
      ::llvm::Value* src_address);
  ::llvm::Value* CloneObject(
      State* state, const TypeSpec& type_spec, ::llvm::Value* src_address);
  // If "result' does not have a memory address, create a temporary variable and
  // copy the value there. Otherwise, do nothing.
  void EnsureAddress(State* state, ExprResult* result);

  // Generates code to deallocate temps in a scope.
  void DestroyTemps(State* state);
  // Generates code to deallocate variables in a scope.
  void DestroyScope(State* state, Scope* scope);
  // Generates code to deallocate variables in all scopes up to and including
  // the function's root scope. Does NOT pop off the scopes.
  void DestroyFnScopes(State* state);

  ::llvm::LLVMContext ctx_;
  struct {
    TypeSpec object_type_spec;
    ::llvm::StructType* object_ty;
    TypeSpec int32_type_spec;
    TypeSpec int_type_spec;
    ::llvm::StructType* int32_ty;
    TypeSpec bool_type_spec;
    ::llvm::StructType* bool_ty;
    TypeSpec string_type_spec;
    ::llvm::StructType* string_ty;
  } builtin_types_;
  ::std::unordered_map<::std::string, ::llvm::Type*> builtin_types_map_;
};

}  // namespace quo

#endif  // IR_GENERATOR_HPP_
