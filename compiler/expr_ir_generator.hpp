/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2017 Chuan Ji <ji@chu4n.com>                               *
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

#ifndef EXPR_IR_GENERATOR_HPP_
#define EXPR_IR_GENERATOR_HPP_

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "ast/ast.pb.h"

namespace quo {

class Builtins;
class Symbols;

// The result of IR generation for an expression. Only a subset of the fields
// will be set for any given expression.
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

// Implements AST -> IR generation for expressions.
class ExprIRGenerator {
 public:
  ExprIRGenerator(
      ::llvm::Module* module,
      ::llvm::IRBuilder<>* ir_builder,
      const Builtins* builtins,
      Symbols* symbols);

  // Run AST -> IR generation for an expression.
  ExprResult ProcessExpr(const Expr& expr);

  // If "result' does not have a memory address, create a temporary variable and
  // copy the value there. Otherwise, do nothing.
  void EnsureAddress(ExprResult* result);
  // Extracts a bool (int8) value from a QBool value.
  ::llvm::Value* ExtractBoolValue(::llvm::Value* wrapped_bool_value);
  // Assigns the object at src_address to the variable referenced by
  // dest_ref_address.
  //
  // This will decrement the reference count of the object currently referenced
  // by dest_ref_address, and increment the reference count of the object at
  // src_address.
  //
  // Type of dest_ref_address: QObject**
  // Type of src_address: QObject*
  ::llvm::Value* AssignObject(
      const TypeSpec& type_spec,
      ::llvm::Value* dest_ref_address,
      ::llvm::Value* src_address);

 private:
  ExprResult ProcessConstantExpr(const ConstantExpr& expr);
  ExprResult ProcessVarExpr(const VarExpr& expr);
  ExprResult ProcessCallExpr(const CallExpr& expr);
  ExprResult ProcessBinaryOpExpr(const BinaryOpExpr& expr);
  ExprResult ProcessAssignExpr(const AssignExpr& expr);

  ::llvm::Value* CreateInt32Value(::llvm::Value* raw_int32_value);
  ::llvm::Value* ExtractInt32Value(::llvm::Value* wrapped_int32_value);
  ::llvm::Value* CreateBoolValue(::llvm::Value* raw_bool_value);
  ::llvm::Value* CreateObject(const TypeSpec& type_spec);
  ::llvm::Value* CreateObject(
      const TypeSpec& type_spec, ::llvm::Value* init_value);
  ::llvm::Value* CloneObject(
      const TypeSpec& type_spec, ::llvm::Value* src_address);

  ::llvm::LLVMContext& ctx_;
  ::llvm::Module* const module_;
  ::llvm::IRBuilder<>* const ir_builder_;
  const Builtins* const builtins_;
  Symbols* const symbols_;
};

}  // namespace quo

#endif  // EXPR_IR_GENERATOR_HPP_
