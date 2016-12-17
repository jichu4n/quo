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

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
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
    TypeSpec type_spec;
    ::llvm::Value* value;
    ::llvm::Value* address;
    const FnDef* fn_def;
  };
  struct Var {
    ::std::string name;
    TypeSpec type_spec;
    ::llvm::Value* address;
  };
  // Represents a single layer of variable scope.
  struct Scope {
    // Addresses of variable in this scope in insertion order.
    ::std::list<Var> vars;
    // Map from name to variables.
    ::std::unordered_map<::std::string, Var*> vars_by_name;
    // Map from address to names.
    ::std::unordered_map<::llvm::Value*, Var*> vars_by_address;
    // Addresses of temps in this scope in insertion order.
    ::std::list<::llvm::Value*> temps;

    // Adds a variable into the scope.
    void AddVar(const Var& var);
    // Adds a temp into the scope.
    void AddTemp(::llvm::Value* address);

    // Look up a variable's address by name. Returns nullptr if not found.
    const Var* Lookup(const ::std::string& name);
    // Look up a variable's name by address. Returns nullptr if not found.
    const Var* Lookup(::llvm::Value* address);
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
